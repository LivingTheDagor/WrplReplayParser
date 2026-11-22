#include "state/ParserState.h"
#include "mpi/GMSyncPrescan.h"
#include "mpi/GMSyncWalk.h"
#include "mpi/GeneralObject.h"
#include "mpi/PositionSync.h"
#include "Replay/ReplayReader.h"
#include "daNet/delta/rle.h"
#include <atomic>
#include <thread>
#if defined(__linux__)
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <memory>
#include <vector>
#include <cstring>
#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace gmsync {
  namespace {
    // Memory handed out upwards, in blocks that never move (the parser reads what was handed out earlier
    // while the thread goes on).
    class Arena {
      static constexpr size_t BLOCK = size_t(1) << 20;
      std::vector<std::unique_ptr<uint8_t[]>> blocks;
      uint8_t *cur = nullptr;
      uint8_t *end = nullptr;

    public:
      /// the address the next bytes go to, with room for `size` of them. Nothing is taken yet.
      uint8_t *room(size_t size) {
        if (size_t(end - cur) < size) {
          const size_t block = size > BLOCK ? size : BLOCK;
          blocks.emplace_back(new uint8_t[block]);
          cur = blocks.back().get();
          end = cur + block;
        }
        return cur;
      }
      void take(size_t size) { cur += size; }
      /// room(size), at an address that is a multiple of 8
      uint8_t *roomAligned(size_t size) {
        room(size + 8);
        cur += (8 - (reinterpret_cast<uintptr_t>(cur) & 7)) & 7;
        return cur;
      }
    };

    // The prescan only helps while a core has nothing better to do. When the parser and the thread that
    // decodes the next part already fill the cores there are, a third thread at the same priority takes
    // time from the parser, the one everything waits for. On Linux a thread can have a nice value of its
    // own: at 19 it gets what the others leave. Elsewhere it runs at the normal priority.
    void yield_to_other_threads() {
#if defined(__linux__)
      setpriority(PRIO_PROCESS, (id_t) syscall(SYS_gettid), 19);
#endif
    }

    int current_cpu() {
#if defined(__linux__)
      return sched_getcpu();
#else
      return -1;
#endif
    }

    // The history thread and the parser wait for each other often, a message at a time. On the same core one
    // of them waits for the other's time slice to end (milliseconds), so keep the history thread off the core
    // the parser was on when it started (when the process may use more than that one).
    void keep_off_cpu(int cpu) {
#if defined(__linux__)
      if (cpu < 0 || cpu >= CPU_SETSIZE)
        return;
      cpu_set_t set;
      CPU_ZERO(&set);
      if (sched_getaffinity(0, sizeof(set), &set) != 0 || !CPU_ISSET(cpu, &set) || CPU_COUNT(&set) < 2)
        return;
      CPU_CLR(cpu, &set);
      sched_setaffinity(0, sizeof(set), &set);
#else
      (void) cpu;
#endif
    }

    inline void cpu_relax() {
#if defined(__x86_64__) || defined(_M_X64)
      _mm_pause();
#elif defined(__aarch64__)
      asm volatile("yield");
#endif
    }

    constexpr uint32_t FINISHED = 0x80000000u; ///< in Prescan::histProgress: the history thread is done
  } // namespace

    class Prescan final : public replay::IPartSidecar {
    public:
      Prescan(const uint8_t *part, std::span<const replay::PacketRef> packets) {
        // The sync messages of the part: what mpi::dispatch and TankMessage::readPayload make of a packet.
        for (const replay::PacketRef &p: packets) {
          if (p.type != replay::ReplayPacketType::MPI)
            continue;
          const danet::BitStream bs(part + p.offset, p.size, false);
          mpi::ObjectID oid;
          mpi::ObjectExtUID extUid;
          mpi::MessageID mid;
          if (!mpi::read_object_ext_uid(bs, oid, extUid) || !bs.Read(mid))
            continue;
          if (mid != MPI_PACKETS::Tank1 && mid != MPI_PACKETS::Tank2)
            continue;
          const uint32_t bytesRead = BITS_TO_BYTES(bs.GetReadOffset());
          const danet::BitStream payload(bs.GetData() + bytesRead, bs.GetNumberOfBytesUsed() - bytesRead, false);
          danet::BitStream data;
          if (!payload.ReadView(data))
            continue;
          PrescanMsg &m = msgs.emplace_back();
          m.data = data.GetData();
          m.bits = data.GetNumberOfBitsUsed();
        }
        readyFrom.store(uint32_t(msgs.size()), std::memory_order_relaxed);
        if (!msgs.empty()) {
          try {
            worker = std::thread([this] { run(); });
          } catch (...) {
            // no thread: nothing is ever ready, the parser reads everything itself
          }
        }
      }

      ~Prescan() override {
        // a part that goes away before the parser has taken all of it: the histories are set back to what
        // the parser has taken
        if (bound)
          recover();
        stop.store(true, std::memory_order_relaxed);
        if (worker.joinable())
          worker.join();
      }

      /// see gmsync::take; parsing thread
      const ReadyMsg *take(net_delta_t &nd, const uint8_t *data, uint32_t bits, bool inline_only) {
        if (mode == Mode::Unbound) {
          if (nd.gmsync_link)
            nd.gmsync_link->recover(); // a part that is still bound to it: should not happen
          // Not worth a thread: a part with few syncs, or when the thread could not keep up with the parser in
          // a part not long ago (ships: a lot of data per vehicle, little else to parse). Then the parser does
          // it, as without the thread.
          const bool paused = nd.gmsync_pause > 0;
          if (paused)
            nd.gmsync_pause--;
          if (inline_only || msgs.size() < MIN_MSGS || paused) {
            mode = Mode::Inline;
            return nullptr;
          }
          bind(nd);
        }
        if (mode == Mode::Inline)
          return nullptr;
        if (&nd != bound || inline_only) {
          recover();
          return nullptr;
        }
        if (mode == Mode::Starting) {
          // The thread has not started yet (a new thread can take a while to get a core): the parser reads
          // the messages itself until the thread is there, then hands the histories over at the message it
          // is at.
          uint32_t at = next;
          while (at < msgs.size() && msgs[at].data < data)
            ++at;
          if (at >= msgs.size() || msgs[at].data != data || msgs[at].bits != bits)
            return nullptr; // not one of the messages of the list
          next = at;
          if (histState.load(std::memory_order_acquire) != HistState::Awake) {
            next = at + 1; // read by the parser
            return nullptr;
          }
          startAt = at;
          histState.store(HistState::Go, std::memory_order_release);
          histState.notify_one();
          mode = Mode::Bound;
        }
        if (next >= msgs.size() || msgs[next].data != data || msgs[next].bits != bits) {
          // not the message that comes next: stop here, the parser goes on by itself
          recover();
          return nullptr;
        }
        const uint32_t at = next;
        uint32_t v = histProgress.load(std::memory_order_acquire);
        if ((v & ~FINISHED) <= at && !(v & FINISHED)) {
          // The thread does not keep up (ships: a lot of data per vehicle, and little else for the parser to
          // do): stop it, the parser goes on faster by itself. (What the thread did past this message is set
          // back, see recover.)
          if (++waits >= 64 && size_t(waits) * 16 > size_t(at - startAt)) {
            recover();
            return nullptr;
          }
        }
        // The thread is a message or so ahead at worst: spin, without giving the core away (a thread that does
        // can wait a whole scheduler tick, milliseconds, to get it back). After a while, sleep.
        for (uint32_t spin = 0; (v & ~FINISHED) <= at && !(v & FINISHED); spin++) {
          if (spin < (1u << 16))
            cpu_relax();
          else
            histProgress.wait(v, std::memory_order_acquire);
          v = histProgress.load(std::memory_order_acquire);
        }
        if ((v & ~FINISHED) <= at) {
          // the thread stopped in front of this message (one it cannot read)
          recover();
          return nullptr;
        }
        ++next;
        return &ready[at];
      }

      /// The history thread stops; the histories of the bound net_delta_t are set to what the parser has taken
      /// (the thread may have been further on). Then the part is read by the parser from `next` on.
      void recover() {
        G_ASSERT(bound);
        stopHistory();
        net_delta_t &nd = *bound;
        const uint32_t done = histProgress.load(std::memory_order_relaxed) & ~FINISHED;
        if (snapshotTaken && (done != next || histBroken)) {
          // back to where the thread started, then the messages the parser has taken from there once more
          restoreSnapshot(nd);
          for (uint32_t i = startAt; i < next; i++) {
            const PrescanMsg &m = histMsgs[i];
            for (uint32_t k = 0; k < m.count; k++) {
              const PrescanRec &r = m.recs[k];
              auto &hist = nd.getHistory(r.uid);
              if (!r.headerOk)
                continue;
              net::DeltaComp::DeltaHeader h;
              h.fullDiff = r.fullDiff;
              h.packetNoDelta = r.packetNoDelta;
              h.nextPacketNo = r.nextPacketNo;
              h.compressedSize = r.packedSize;
              nd.netDelta.applyDelta(h, r.packed, &hist, r.unpacked, r.unpackedSize);
            }
          }
        }
        // the parser had to wait for more than one message in 16: leave the next parts to the parser, more of
        // them every time
        if (size_t(waits) * 16 > size_t(next - startAt)) {
          nd.gmsync_pause =
            uint8_t(std::min<uint32_t>(255u, uint32_t(PAUSE_PARTS) << std::min<uint32_t>(nd.gmsync_bad, 6u)));
          if (nd.gmsync_bad < 255)
            nd.gmsync_bad++;
        }
        nd.gmsync_link = nullptr;
        bound = nullptr;
        mode = Mode::Inline;
      }

      void stopHistory() {
        histStop.store(true, std::memory_order_relaxed);
        histState.store(HistState::Stop, std::memory_order_release);
        histState.notify_one();
        if (histWorker.joinable())
          histWorker.join();
      }

      /// the net_delta_t goes away: stop, nothing else
      void detach() {
        stopHistory();
        bound->gmsync_link = nullptr;
        bound = nullptr;
        mode = Mode::Inline;
      }

      const PrescanMsg *find(const uint8_t *data, uint32_t bits) const {
        // messages are in packet order, which is address order, and the parser asks in that order too
        uint32_t at = hint;
        const uint32_t count = uint32_t(msgs.size());
        while (at < count && msgs[at].data < data)
          ++at;
        hint = at;
        if (at >= count || msgs[at].data != data)
          return nullptr;
        hint = at + 1;
        // the thread stops in front of this message: from here on down the parser is first
        if (parserAt.load(std::memory_order_relaxed) < at + 1)
          parserAt.store(at + 1, std::memory_order_relaxed);
        if (at < readyFrom.load(std::memory_order_acquire))
          return nullptr;
        const PrescanMsg &m = msgs[at];
        return m.usable && m.bits == bits ? &m : nullptr;
      }

    private:
      struct Sink {
        const net::DeltaComp &comp;
        std::vector<PrescanRec> &recs;
        Arena &arena;
        const uint8_t *msgEnd;
        bool gaveUp = false;

        const net::DeltaComp &deltaComp() const { return comp; }

        bool delta(uint16_t uid, const Unit &unit, bool header_ok, const net::DeltaComp::DeltaHeader &h,
                   const uint8_t *packed) {
          PrescanRec &r = recs.emplace_back();
          r.packed = packed;
          r.unpacked = nullptr;
          r.unpackedSize = 0;
          r.nextPacketNo = h.nextPacketNo;
          r.packetNoDelta = h.packetNoDelta;
          r.packedSize = h.compressedSize;
          r.uid = uid;
          r.turret_count = unit.turret_count;
          r.val1 = unit.val1;
          r.headerOk = header_ok;
          r.fullDiff = h.fullDiff;
          // unpack it as net::delta::decompress does (room for 10 times the packed size), straight into the
          // memory the parser will read it from
          if (header_ok && !h.fullDiff && packed + h.compressedSize <= msgEnd) {
            const size_t room = size_t(h.compressedSize) * 10;
            uint8_t *const to = arena.room(room + 1);
            const int len = net::delta::rle0ki_decompress(
              std::span<uint8_t>(to, room), std::span<uint8_t>(const_cast<uint8_t *>(packed), h.compressedSize));
            if (len >= 0) {
              arena.take(size_t(len));
              r.unpacked = to;
              r.unpackedSize = uint32_t(len);
            }
          }
          return true;
        }

        // data that is not delta packed: how much of it is read depends on the parser state
        bool plain(danet::BitCursor &, uint16_t, const Unit &) {
          gaveUp = true;
          return false;
        }
      };

      static void prescan(PrescanMsg &m, const net::DeltaComp &comp, std::vector<PrescanRec> &recs, Arena &arena) {
        recs.clear();
        Sink sink{comp, recs, arena, m.data + BITS_TO_BYTES(m.bits)};
        danet::BitCursor cursor{m.data, m.bits};
        // The four fields in front of the first vehicle are read without a check: a message too short for
        // them is left to the parser, as is one with data that is not delta packed.
        const bool whole = m.bits >= 80;
        m.result = whole ? walk(cursor, sink) : false;
        m.endOffset = cursor.GetReadOffset();
        m.count = uint32_t(recs.size());
        if (m.count) {
          const size_t bytes = recs.size() * sizeof(PrescanRec);
          uint8_t *const to = arena.roomAligned(bytes);
          memcpy(to, recs.data(), bytes);
          arena.take(bytes);
          m.recs = reinterpret_cast<const PrescanRec *>(to);
        }
        m.usable = whole && !sink.gaveUp;
      }

      void run() {
        yield_to_other_threads();
        try {
          const net::DeltaComp comp(net_delta_t::HISTORY_BITS, net_delta_t::INDEX_BITS, nullptr);
          std::vector<PrescanRec> recs;
          // From the last message towards the first, a batch at a time; inside a batch forwards, so that what
          // the parser reads for consecutive messages is consecutive in memory.
          constexpr uint32_t BATCH = 32;
          for (uint32_t hi = uint32_t(msgs.size()); hi > 0;) {
            const uint32_t lo = hi > BATCH ? hi - BATCH : 0;
            for (uint32_t i = lo; i < hi; i++) {
              if (stop.load(std::memory_order_relaxed))
                return;
              if (i < parserAt.load(std::memory_order_relaxed))
                continue; // the parser was here first (the message stays "not usable")
              prescan(msgs[i], comp, recs, arena);
            }
            readyFrom.store(lo, std::memory_order_release);
            if (lo < parserAt.load(std::memory_order_relaxed))
              break; // met the parser
            hi = lo;
          }
        } catch (...) {
          // out of memory: what is published stays valid, the parser does the rest
        }
      }

      enum class Mode : uint8_t { Unbound, Starting, Bound, Inline };
      enum class HistState : uint32_t { Requested, Awake, Go, Stop }; // 32 bits: waited for as a futex

      void bind(net_delta_t &nd) {
        bound = &nd;
        nd.gmsync_link = this;
        mode = Mode::Starting;
        histMsgs.resize(msgs.size());
        ready.resize(msgs.size());
        try {
          const int parser_cpu = current_cpu();
          histWorker = std::thread([this, parser_cpu] {
            keep_off_cpu(parser_cpu);
            runHistory();
          });
        } catch (...) {
          // no thread: the parser does it all
          bound = nullptr;
          nd.gmsync_link = nullptr;
          mode = Mode::Inline;
        }
      }

      /// The history thread: every message from the first on, as GMSync would do it, apart from the state.
      void runHistory() {
        // tell the parser this thread is there, and wait for it to hand over (at its next sync message)
        HistState st = HistState::Requested;
        if (!histState.compare_exchange_strong(st, HistState::Awake, std::memory_order_acq_rel)) {
          histProgress.store(FINISHED, std::memory_order_release);
          return; // stopped before it began
        }
        // Spin (giving the core away in between): the parser comes to a sync message soon, and a thread that
        // sleeps can take a whole scheduler tick to get a core back.
        for (int spin = 0; (st = histState.load(std::memory_order_acquire)) == HistState::Awake; spin++) {
          if (spin < 20000)
            std::this_thread::yield();
          else
            histState.wait(HistState::Awake, std::memory_order_acquire);
        }
        if (st != HistState::Go) {
          histProgress.store(FINISHED, std::memory_order_release);
          return;
        }
        uint32_t i = startAt;
        try {
          net_delta_t &nd = *bound;
          takeSnapshot(nd);
          snapshotTaken = true;
          std::vector<PrescanRec> recs;
          std::vector<ReadyUnit> units;
          std::vector<TurretSet> turrets;
          for (; i < uint32_t(msgs.size()); i++) {
            if (histStop.load(std::memory_order_relaxed))
              break;
            // the prescan thread stops in front of this message
            if (parserAt.load(std::memory_order_relaxed) < i + 1)
              parserAt.store(i + 1, std::memory_order_relaxed);
            PrescanMsg &m = histMsgs[i];
            if (i >= readyFrom.load(std::memory_order_acquire) && msgs[i].usable) {
              m = msgs[i]; // done by the prescan thread (records in its arena, which stays)
            } else {
              // not done by the prescan thread (or left out by it because this thread had got there first)
              m.data = msgs[i].data;
              m.bits = msgs[i].bits;
              prescan(m, nd.netDelta, recs, histArena);
            }
            if (!m.usable)
              break; // the parser reads it (and the rest of the part)
            units.clear();
            turrets.clear();
            for (uint32_t k = 0; k < m.count; k++) {
              const PrescanRec &r = m.recs[k];
              auto &hist = nd.getHistory(r.uid);
              ReadyUnit &u = units.emplace_back();
              u.uid = r.uid;
              u.parsed = false;
              if (!r.headerOk)
                continue;
              net::DeltaComp::DeltaHeader h;
              h.fullDiff = r.fullDiff;
              h.packetNoDelta = r.packetNoDelta;
              h.nextPacketNo = r.nextPacketNo;
              h.compressedSize = r.packedSize;
              const auto res = nd.netDelta.applyDelta(h, r.packed, &hist, r.unpacked, r.unpackedSize);
              if (res.ok) {
                u.parsed = true;
                dataSync::decodeVehicleInfo(res.data, res.sizeBytes, r.val1, r.turret_count, u.info, turrets);
              }
            }
            ReadyMsg &out = ready[i];
            out.count = uint32_t(units.size());
            out.endOffset = m.endOffset;
            out.result = m.result;
            if (!units.empty()) {
              const size_t bytes = units.size() * sizeof(ReadyUnit);
              uint8_t *const to = histArena.roomAligned(bytes);
              memcpy(to, units.data(), bytes);
              histArena.take(bytes);
              out.units = reinterpret_cast<const ReadyUnit *>(to);
            }
            if (!turrets.empty()) {
              const size_t bytes = turrets.size() * sizeof(TurretSet);
              uint8_t *const to = histArena.roomAligned(bytes);
              memcpy(to, turrets.data(), bytes);
              histArena.take(bytes);
              out.turrets = reinterpret_cast<const TurretSet *>(to);
            }
            histProgress.store(i + 1, std::memory_order_release);
            histProgress.notify_one();
          }
        } catch (...) {
          // out of memory, maybe in the middle of a message: the parser sets the histories back (recover)
          histBroken = true;
        }
        histProgress.store(i | FINISHED, std::memory_order_release);
        histProgress.notify_one();
      }

      std::vector<PrescanMsg> msgs;
      Arena arena;
      // the history thread (see gmsync::take)
      Mode mode = Mode::Unbound; ///< parsing thread
      uint32_t next = 0; ///< the message the parser takes next; parsing thread
      net_delta_t *bound = nullptr;
      // The histories where the thread starts, to set them back to (recover). Only what a history is made of
      // when it is read: the used bytes of every slot, not the memory behind them, which is a lot more.
      struct HistSnap {
        uint32_t firstSlot, slots;
        uint32_t curPacketTotalNo, curPacketNo;
        bool isEnabled;
        net::DeltaComp::History::Stats stats;
      };
      struct SlotSnap {
        uint32_t bits;
        uint32_t bytes;
        size_t at;
      };
      std::vector<HistSnap> snapHist;
      std::vector<SlotSnap> snapSlots;
      std::vector<uint8_t> snapBytes;
      std::vector<net::DeltaComp::History> snapFull; ///< all of it, if a history has a cache (not when reading)
      bool snapIsFull = false;

      void takeSnapshot(const net_delta_t &nd) {
        for (const auto &h: nd.histories)
          if (!h.compressedCache.empty()) {
            snapFull = nd.histories;
            snapIsFull = true;
            return;
          }
        snapHist.reserve(nd.histories.size());
        for (const auto &h: nd.histories) {
          HistSnap &hs = snapHist.emplace_back();
          hs.firstSlot = uint32_t(snapSlots.size());
          hs.slots = uint32_t(h.baseHist.size());
          hs.curPacketTotalNo = h.curPacketTotalNo;
          hs.curPacketNo = h.curPacketNo;
          hs.isEnabled = h.isEnabled;
          hs.stats = h.stats;
          for (const auto &b: h.baseHist) {
            SlotSnap &ss = snapSlots.emplace_back();
            ss.bits = b.GetNumberOfBitsUsed();
            ss.bytes = ss.bits ? b.GetNumberOfBytesUsed() : 0;
            ss.at = snapBytes.size();
            if (ss.bytes)
              snapBytes.insert(snapBytes.end(), b.GetData(), b.GetData() + ss.bytes);
          }
        }
      }

      void restoreSnapshot(net_delta_t &nd) {
        if (snapIsFull) {
          nd.histories = std::move(snapFull);
          return;
        }
        nd.histories.resize(snapHist.size());
        for (size_t i = 0; i < snapHist.size(); i++) {
          const HistSnap &hs = snapHist[i];
          auto &h = nd.histories[i];
          h.baseHist.resize(hs.slots);
          for (uint32_t j = 0; j < hs.slots; j++) {
            const SlotSnap &ss = snapSlots[hs.firstSlot + j];
            danet::BitStream &b = h.baseHist[j];
            b.SetWriteOffset(0);
            if (ss.bytes) {
              constexpr uint32_t PAD = net::DeltaComp::DeltaView::PADDING;
              b.reserveBits(BYTES_TO_BITS(ss.bytes + PAD));
              memcpy(b.GetData(), snapBytes.data() + ss.at, ss.bytes);
              memset(b.GetData() + ss.bytes, 0, PAD);
            }
            b.SetWriteOffset(ss.bits);
          }
          h.curPacketTotalNo = hs.curPacketTotalNo;
          h.curPacketNo = hs.curPacketNo;
          h.isEnabled = hs.isEnabled;
          h.stats = hs.stats;
        }
      }
      std::vector<PrescanMsg> histMsgs; ///< what the history thread read of every message
      std::vector<ReadyMsg> ready;
      Arena histArena;
      std::atomic<uint32_t> histProgress{0}; ///< messages done, | FINISHED at the end
      std::atomic<bool> histStop{false};
      bool histBroken = false;
      bool snapshotTaken = false;
      uint32_t waits = 0; ///< messages the parser had to wait for; parsing thread
      static constexpr size_t MIN_MSGS = 1024;
      static constexpr uint8_t PAUSE_PARTS = 4;
      uint32_t startAt = 0; ///< the first message of the thread (written before Go)
      std::atomic<HistState> histState{HistState::Requested};
      std::thread histWorker;
      std::atomic<uint32_t> readyFrom{0}; ///< msgs[i] is finished for every i >= readyFrom
      mutable std::atomic<uint32_t> parserAt{0}; ///< the parser has reached every message below this
      std::atomic<bool> stop{false};
      mutable uint32_t hint = 0; ///< parser thread only
      std::thread worker;
    };

  namespace {
    std::unique_ptr<replay::IPartSidecar> make(const uint8_t *part, std::span<const replay::PacketRef> packets) {
      return std::make_unique<Prescan>(part, packets);
    }
  } // namespace

  const ReadyMsg *take(net_delta_t &nd, const replay::IPartSidecar *sidecar, const uint8_t *data, uint32_t bits,
                       bool inline_only) {
    if (!sidecar)
      return nullptr;
    Prescan *prescan = const_cast<Prescan *>(dynamic_cast<const Prescan *>(sidecar));
    return prescan ? prescan->take(nd, data, bits, inline_only) : nullptr;
  }

  void detach(net_delta_t &nd) {
    if (nd.gmsync_link)
      nd.gmsync_link->detach();
  }

  const PrescanMsg *lookup(const replay::IPartSidecar *sidecar, const uint8_t *data, uint32_t bits) {
    if (!sidecar)
      return nullptr;
    const Prescan *prescan = dynamic_cast<const Prescan *>(sidecar);
    return prescan ? prescan->find(data, bits) : nullptr;
  }

  void enable(bool on) { replay::set_part_sidecar_factory(on ? &make : nullptr); }
} // namespace gmsync
