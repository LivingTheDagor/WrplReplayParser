#include "state/ParserState.h"
#include "mpi/GMSyncPrescan.h"
#include "mpi/GMSyncWalk.h"
#include "mpi/GeneralObject.h"
#include "Replay/ReplayReader.h"
#include "daNet/delta/rle.h"
#include <atomic>
#include <thread>
#if defined(__linux__)
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <memory>
#include <vector>
#include <cstring>

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
        stop.store(true, std::memory_order_relaxed);
        if (worker.joinable())
          worker.join();
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

      void prescan(PrescanMsg &m, const net::DeltaComp &comp, std::vector<PrescanRec> &recs) {
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
              prescan(msgs[i], comp, recs);
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

      std::vector<PrescanMsg> msgs;
      Arena arena;
      std::atomic<uint32_t> readyFrom{0}; ///< msgs[i] is finished for every i >= readyFrom
      mutable std::atomic<uint32_t> parserAt{0}; ///< the parser has reached every message below this
      std::atomic<bool> stop{false};
      mutable uint32_t hint = 0; ///< parser thread only
      std::thread worker;
    };

    std::unique_ptr<replay::IPartSidecar> make(const uint8_t *part, std::span<const replay::PacketRef> packets) {
      return std::make_unique<Prescan>(part, packets);
    }
  } // namespace

  const PrescanMsg *lookup(const replay::IPartSidecar *sidecar, const uint8_t *data, uint32_t bits) {
    if (!sidecar)
      return nullptr;
    const Prescan *prescan = dynamic_cast<const Prescan *>(sidecar);
    return prescan ? prescan->find(data, bits) : nullptr;
  }

  void enable(bool on) { replay::set_part_sidecar_factory(on ? &make : nullptr); }
} // namespace gmsync
