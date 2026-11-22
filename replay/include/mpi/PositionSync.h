#pragma once

struct ParserState;
namespace gmsync {
  struct Unit;
}
namespace net {
  struct DeltaComp;
}
namespace danet {
  template<bool Padded>
  class BitCursorT;
  using BitCursor = BitCursorT<false>;
  using PaddedBitCursor = BitCursorT<true>;
} // namespace danet
typedef Rocket *(*get_weapon_cb)(ParserState &state, ecs::EntityId eid);
struct dataSync {
  static bool FMSync(ParserState &state, danet::BitStream &bs);
  static bool GMSync(ParserState &state, danet::BitStream &bs);
  static bool WeaponSync(ParserState &state, danet::BitStream &bs);

  struct AmmoSkip {
    uint32_t offset; ///< bit position behind the records that were stepped over
    uint32_t records; ///< how many records that was
  };
  /// The ammunition block of a ground-model sync has one record per turret: a single 0 bit, or a 1 bit,
  /// 72 bits, a count (8 bits) and `count` 16-bit values. Nothing in it is used, it only has to be stepped
  /// over, and a ship has up to 160 turrets in every sync.
  /// Steps over the records at bit `offset` of `data` (`bits_used` bits long), at most `max_records` of them,
  /// as long as a record is whole and starts at least 97 bits before the end. What it leaves is for the
  /// reads of GMSync, one field at a time, which also deal with a stream that ends inside a record.
  static AmmoSkip skipAmmoRecords(const uint8_t *data, uint32_t bits_used, uint32_t offset, uint32_t max_records);

private:
  struct TankRef {
    unit::Unit *ref_1 = nullptr; // + 0.0 always matches
    unit::Unit *ref_2 = nullptr; // + 0x8 needs to not match flags 0x10 and 0x1800
    bool val1 = false;
    bool val2 = false;
    bool val3 = false;
    uint8_t turret_count = 0;
  };
  static bool GMSyncImpl(ParserState &state, danet::BitCursor &bs);
  struct GMSyncSink;
  /// One vehicle of a sync with a delta packet: bring its history up to the packet and parse the result.
  /// `unpacked`: see net::DeltaComp::applyDelta. Returns false when the message cannot be read on.
  static bool unitDelta(ParserState &state, uint16_t uid, const gmsync::Unit &unit, bool header_ok,
                        uint32_t next_packet_no, uint32_t packet_no_delta, bool full_diff, uint16_t packed_size,
                        const uint8_t *packed, const uint8_t *unpacked, uint32_t unpacked_size);
  /// one vehicle of a sync whose data is not delta packed, at the position of `bs`
  static bool unitPlain(ParserState &state, danet::BitCursor &bs, uint16_t uid, const gmsync::Unit &unit);
  static bool ParseVehicleInfo(ParserState &state, danet::PaddedBitCursor &bs, TankRef *ref, bool is_from_compress);
  static bool ParseVehicleInfoImpl(ParserState &state, danet::PaddedBitCursor &bs, TankRef *ref,
                                   bool is_from_compress);
  static bool ParseWeapon(ParserState &state, const danet::BitStream &bs, get_weapon_cb cb);
};
