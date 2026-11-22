#pragma once
#include <cstdint>

namespace replay {
  class IPartSidecar;
}

/// Ground-model syncs, read ahead of the parser.
///
/// Most of the work of a sync does not need the parser: finding the vehicles in the message and unpacking the
/// delta packet of each (rle0ki) depend on the bytes of the message only. With this on, every decoded part of
/// a replay gets a thread that does that for its sync messages, from the last message of the part towards the
/// first, while the parser works from the first towards the last. A message the thread has finished costs the
/// parser one XOR per vehicle; a message it has not reached (or the two meeting in the middle) is read by the
/// parser itself, as before. The parser never waits, and the results are the same bytes either way.
namespace gmsync {
  /// one vehicle of a message
  struct PrescanRec {
    const uint8_t *packed; ///< packed bytes of the delta packet, inside the message
    const uint8_t *unpacked; ///< what they unpack to, or nullptr (full packet, no header): then the parser unpacks
    uint32_t unpackedSize;
    uint32_t nextPacketNo;
    uint32_t packetNoDelta;
    uint16_t packedSize;
    uint16_t uid;
    uint8_t turret_count;
    bool val1;
    bool headerOk;
    bool fullDiff;
  };
  /// one sync message
  struct PrescanMsg {
    const uint8_t *data; ///< the sync data (what GMSync is given)
    uint32_t bits;
    uint32_t count = 0; ///< vehicles with a delta packet, in message order
    const PrescanRec *recs = nullptr;
    uint32_t endOffset = 0; ///< read position at the end of the walk
    bool result = false; ///< what the walk returned
    bool usable = false; ///< false: the message has to be read by the parser
  };

  /// The finished prescan of the message whose sync data is `data`/`bits`, or nullptr: not a part with a
  /// prescan, not one of its messages, not done yet, or not usable. Call it once per message, from the thread
  /// that parses (it also tells the prescan thread how far the parser is).
  const PrescanMsg *lookup(const replay::IPartSidecar *sidecar, const uint8_t *data, uint32_t bits);

  /// Process wide switch (off at start, initialize() turns it on, see InitOptions::sync_prescan). Parts
  /// decoded from then on get a prescan, or none.
  void enable(bool on);
} // namespace gmsync
