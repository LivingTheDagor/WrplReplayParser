//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vector>
#include <cstdint>
#include <math/dag_adjpow2.h>
#include <daNet/bitStream.h>
#include <daNet/bitCursor.h>

#define VALIDATE_COMPRESSION_HASH 0 // NOTE: used for debug only, changing this changes protocol

namespace net {

  struct DeltaComp {
    uint32_t historyBits;
    uint32_t indexBits;
    IMemAlloc *allocator;

    struct History {
      std::vector<danet::BitStream> baseHist;
      struct CacheEntry {
        danet::BitStream bs;
        bool isFullDiff = false;
        uint32_t basePacketNo = 0;
        uint32_t nextPacketNo = 0;
        bool isEmpty() const { return bs.GetWriteOffset() == 0; }
        void clear() { bs.SetWriteOffset(0); }
      };
      std::vector<CacheEntry> compressedCache;
      uint32_t curPacketTotalNo = 0;
      uint32_t curPacketNo = 0;
      bool isEnabled = false;
      bool isInited() const { return !baseHist.empty(); }

      struct Stats {
        bool isHistoryBroken = false;
        uint32_t lastReceivedBasePacketNo = 0;
        uint32_t lastReceivedNextPacketNo = 0;
        uint32_t lastCompressedDataSizeBytes = 0;
      } stats;

      bool isValidBasePacket(uint32_t packet) const {
        return curPacketNo - packet < baseHist.size() &&
               baseHist[packet & (baseHist.size() - 1u)].GetNumberOfBitsUsed() > 0 && isEnabled;
      }

#if 0
      mutable std::vector<uint32_t> dbgPacketHistory;
    void pushDbgPacketHistory(uint32_t packetNo) const
    {
      if (dbgPacketHistory.size() == 1024)
        dbgPacketHistory.pop_back();
      dbgPacketHistory.emplace(dbgPacketHistory.begin(), packetNo);
    }
#else
      void pushDbgPacketHistory(uint32_t) const {}
#endif
    };

    struct State {
      uint32_t lastConfirmedPacketNo : 31 = 0;
      bool validLastConfirmedPacketNo : 1 = false;
      uint32_t lastSentPacketTotalNo = 0;
    };

    /// The fields in front of the packed bytes of one delta packet.
    struct DeltaHeader {
      bool fullDiff = false;
      uint32_t packetNoDelta = 0;
      uint32_t nextPacketNo = 0;
      uint16_t compressedSize = 0;
#if VALIDATE_COMPRESSION_HASH
      uint32_t srvHash = 0u;
#endif
    };

    /// Result of applyDelta: the decoded packet bytes and the ReadResult flags. The bytes are the history slot
    /// of the packet: not owned, valid until the next packet of that history, followed by at least PADDING
    /// loadable bytes (danet::PaddedBitCursor).
    struct DeltaView {
      static constexpr uint32_t PADDING = 8;
      const uint8_t *data = nullptr;
      uint32_t sizeBytes = 0;
      bool ok = false;
      bool isResetNeeded = false;
      bool isOutOfOrder = false;
      bool isFullDiff = false;
      uint32_t confirmPacketNo = 0;
    };

    struct ReadResult {
      /// The decoded packet, not owned: it points into the input stream (full packet) or the history slot (delta).
      /// Valid until the next readDelta on the same history.
      danet::BitStream bs;
      bool ok = false;
      bool isResetNeeded = false;
      bool isOutOfOrder = false;
      bool isFullDiff = false;
      uint32_t confirmPacketNo = 0;
    };

    DeltaComp(uint32_t history_bits, uint32_t index_bits, IMemAlloc *allocator);

    void initHistory(History &history, bool is_enabled, bool use_cache);

    danet::BitStream &writeNextData(History &history);
    const danet::BitStream &getData(const History &history);
    uint32_t getHistoryIdxForPacket(const History &history, uint32_t packet_no);

    void writeDelta(danet::BitStream &bs, History &history, State &state, bool reliable_channel, bool use_cache = true);
    ReadResult readDelta(const danet::BitStream &bs, History *history);

    /// Read the header of one delta packet and move `data` (a BitStream or a BitCursor) past the packet.
    /// `compressed` is set to the packed bytes, h.compressedSize of them. Returns false when a header
    /// field cannot be read.
    template<typename Stream>
    BITCURSOR_INLINE bool readDeltaHeader(Stream &data, DeltaHeader &h, const uint8_t *&compressed) const {
      bool ok = true;
      ok &= data.Read(h.fullDiff);
      ok &= data.ReadBits(reinterpret_cast<uint8_t *>(&h.packetNoDelta), historyBits);
      ok &= data.ReadBits(reinterpret_cast<uint8_t *>(&h.nextPacketNo), indexBits);
      ok &= data.ReadCompressed(h.compressedSize);
#if VALIDATE_COMPRESSION_HASH
      ok &= data.Read(h.srvHash);
#endif
      data.AlignReadToByteBoundary();
      compressed = data.GetData() + BITS_TO_BYTES(data.GetReadOffset());
      data.SetReadOffset(BYTES_TO_BITS(BITS_TO_BYTES(data.GetReadOffset()) + h.compressedSize));
      return ok;
    }
    /// The part of readDelta after the header: bring `history` up to the packet and decode it.
    /// `unpacked`/`unpacked_size`: the delta already unpacked by the caller, or nullptr. Unused for a full packet.
    DeltaView applyDelta(const DeltaHeader &h, const uint8_t *compressed, History *history,
                         const uint8_t *unpacked = nullptr, uint32_t unpacked_size = 0);
    /// readDelta without a stream object for the result.
    template<typename Stream>
    BITCURSOR_INLINE DeltaView readDeltaView(Stream &data, History *history) {
      DeltaHeader h;
      const uint8_t *compressed = nullptr;
      if (!readDeltaHeader(data, h, compressed) || history == nullptr)
        return {};
      return applyDelta(h, compressed, history);
    }
    /// Move the read position of `data` past one delta packet without decoding it. Returns false when the
    /// packet header cannot be read. The history of the sender is not updated, so it cannot be used for
    /// later deltas: the caller has to drop it.
    template<typename Stream>
    BITCURSOR_INLINE bool skipDelta(Stream &data) const {
      DeltaHeader h;
      const uint8_t *compressed = nullptr;
      return readDeltaHeader(data, h, compressed);
    }

    void writeConfirm(danet::BitStream &bs, const ReadResult &result);
    bool readConfirm(const danet::BitStream &bs, const History *history, State *state);
    void resetState(State &state);

    void writeReplaySnapshot(danet::BitStream &bs, const History &history);
    bool readReplaySnapshot(const danet::BitStream &bs, History *history);
  };

} // namespace net
