#pragma once
#include <cstdint>

namespace replay {
  class IPartSidecar;
}
struct net_delta_t;

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

  /// a turret ParseVehicleInfo would set (TurretDesc::setData) for weapon `index`
  struct TurretSet {
    uint16_t index;
    float x, y, z;
  };
  /// What ParseVehicleInfo would do with the data of a vehicle that has a unit (dataSync::decodeVehicleInfo)
  struct VehicleInfo {
    float dir[3] = {};
    float pos[3] = {};
    uint32_t firstTurret = 0; ///< in ReadyMsg::turrets
    uint16_t turretCount = 0;
    bool hasPosition = false; ///< a position sample (tanks only)
    bool reachedEnd = false; ///< the turret update at the end
    bool result = false; ///< what ParseVehicleInfo returns
  };
  /// one vehicle of a ReadyMsg
  struct ReadyUnit {
    uint16_t uid;
    bool parsed; ///< the history gave data (DeltaView::ok); else nothing more is done with the vehicle
    VehicleInfo info;
  };
  /// a sync message that was read all the way, in its order of vehicles
  struct ReadyMsg {
    const ReadyUnit *units = nullptr;
    const TurretSet *turrets = nullptr;
    uint32_t count = 0;
    uint32_t endOffset = 0; ///< read position at the end of the walk
    bool result = false; ///< what GMSync returns
  };

  /// The message whose sync data is `data`/`bits`, read all the way for `nd`, or nullptr: then go on with
  /// lookup() / the parser's own walk, which find `nd` as they need it.
  ///
  /// The first call for a part (from the thread that parses) gives the part a second thread that goes through
  /// its sync messages from the first to the last and does all of GMSync that does not need the state: it
  /// brings the histories of `nd` up to date and reads the vehicle data. While it runs, nothing else may use
  /// the histories of `nd`; the parser takes the messages in order (and waits for one that is not done yet).
  /// The thread stops at a message it cannot read (data that is not delta packed), and when the parser does
  /// not take a message in order (or `inline_only` is true, see ParserConfig skip_sync_without_unit): then
  /// `nd` is set to what the parser has taken so far and this returns nullptr for the rest of the part.
  const ReadyMsg *take(net_delta_t &nd, const replay::IPartSidecar *sidecar, const uint8_t *data, uint32_t bits,
                       bool inline_only);

  /// Process wide switch (off at start, initialize() turns it on, see InitOptions::sync_prescan). Parts
  /// decoded from then on get a prescan, or none.
  void enable(bool on);
} // namespace gmsync
