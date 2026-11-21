#pragma once

struct ParserState;
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
  static bool ParseVehicleInfo(ParserState &state, danet::PaddedBitCursor &bs, TankRef *ref, bool is_from_compress);
  static bool ParseVehicleInfoImpl(ParserState &state, danet::PaddedBitCursor &bs, TankRef *ref,
                                   bool is_from_compress);
  static bool ParseWeapon(ParserState &state, const danet::BitStream &bs, get_weapon_cb cb);
};
