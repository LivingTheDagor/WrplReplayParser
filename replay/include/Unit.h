#pragma once

#include "vector"
#include "cstdint"
#include "translate.h"
#include "danet/BitStream.h"
#include "math/dag_Point3.h"
#include <ioSys/dag_dataBlock.h>
#include "ecs/entityId.h"
#include "math/dag_TMatrix.h"
#include "math/dag_geomTree.h"
#include "mpi/types.h"
#include "mpi/codegen/ReflIncludes.h"
#include "state/StateRewinder.h"
#include "math/dag_mathAng.h"

struct SpaceTime {
  Point3 location{};

  bool operator==(const SpaceTime &other) const { return location == other.location; }
};

struct SpaceTimeEuler : SpaceTime {
  Point3 euler{};

  bool operator==(const SpaceTimeEuler &other) const { return SpaceTime::operator==(other) && euler == other.euler; }
};

struct AngularSpaceTime : SpaceTimeEuler {
  float turret_horizontal{};

  bool operator==(const AngularSpaceTime &other) const {
    return SpaceTimeEuler::operator==(other) && turret_horizontal == other.turret_horizontal;
  }
};

#define COUNTER_MEASURES_COUNT 2
#define SENSORS_COUNT          4
#define TARGETS_NUM            8
#define MAX_WEAPONS_PER_UNIT   256


struct SensorsControlStates {
  // a bunch of this is probably a union actually
  bool v1 = 0;
  bool v2 = 0;
  bool first_bool = false; // maybe is turned on?
  float unpacked_1 = 0;
  float unpacked_2 = 0;
  float unpacked_3 = 0;
  int field149_0xa4 = 0;
  int field150_0xa8 = 0;
  uint8_t field132_0x84 = 0;
  uint8_t field133_0x85 = 0;
  std::vector<uint32_t> field4_0x4{};
  uint8_t sensor_type_maybe = 0;
  uint8_t field136_0x88 = 0;
  uint8_t field137_0x89 = 0;
  uint8_t field138_0x8a = 0;
  float some_data_1{};
  float some_data_2{};
  float some_data_3{};
  float some_data_4{};
  float some_data_5{};
  char some_data_6[4]{};
  int some_data_7;
  float field147_0xa8;

  bool deserialize(BitStream &bs);
};

struct TargetDesignationControlState {
  uint8_t v1;
  uint8_t v2;
  bool v3;
  Point3 v4;
  bool write_compressed;
  float v5;
  Point3 v6;
  Point3 v7;
  bool v8;
  Point3 v9;
  uint8_t v10;
  float v11;
  bool v12;
  bool v13;
  uint8_t v14;
  uint32_t v15;

  bool deserialize(BitStream &bs);
};

struct CounterMeasuresControlState {
  uint8_t v1;
  uint8_t v2;

  bool deserialize(BitStream &bs);
};

enum UnitType : uint8_t {
  TankType = 1,
  AircraftType = 2,

};

struct FieldSerializerDict;

namespace unit {

  std::vector<std::string> getUnitTagsBlk(const DataBlock *blk);

  std::vector<std::string> getUnitTagsName(std::string_view &name);

  /// Name of a weapon class the way a vehicle blk spells it in `trigger`. Not a strict
  /// reverse of get_weapon_id: 0x12 is spelled both targetingPod and gunner1, and this
  /// answers gunner1, as every id above 0x10 seen in a replay has been a gunner mount.
  /// Ground weapons are therefore always gunnerN; class names proper - cannon, atgm,
  /// countermeasures - show up on aircraft. Empty for an id that names no class.
  std::string get_weapon_class(int weapon_id);

  struct TurretNode {
  protected:
    GeomNodeTree::Index16 parent_index;
    GeomNodeTree::Index16 curr_index;
    mutable bool is_useful_node = false;

  protected:
    friend struct TurretTree;

  public:
    Point3 turret_rel{};
    Point3 turret_abs{};
    TurretNode(GeomNodeTree::Index16 idx, GeomNodeTree::Index16 parent) : parent_index(parent), curr_index(idx) {}
  };

  struct TurretTree {
    GeomNodeTree *tree;
    std::vector<TurretNode> nodes;
    std::vector<uint16_t> useful_order;
    bool useful_built = false;
    // names should be stored in tree, so string_view fine
    std::unordered_map<std::string_view, TurretNode *> name_to_idx;

    TurretTree(GeomNodeTree *tree);

    TurretNode *getNode(std::string_view name);
    TurretNode *getUsefulNode(std::string_view name);

    void doAbsUpdate();
  };

  class Aircraft;

  class Tank;

  /// One line of the loadout the player took into the battle: which gun, which bullet
  /// set of it, and how much of it was taken. \see Weapon::bulletSets
  struct weapon_data {
    std::string launcher{}; ///< gun this line is for, by weapon_name; empty on ground, where the gun is the only one
    std::string bullet{}; ///< BulletSet::name of the shell or belt taken
    uint16_t count;
    uint8_t unk;
  };

  /// One thing a loadout can put in a gun: a shell, or a belt of them.
  struct BulletSet {
    std::string name{}; ///< block of the gun blk, as weapon_data::bullet names it; empty for the gun's stock load
    std::string shell{}; ///< the round in it; empty for a belt, which mixes several
    /// Every round the set holds, in blk order: one for a shell, the whole mix for a
    /// belt. A belt is never broken up when fired, so no round of it stands for the
    /// set - but the battle report does name single rounds, and this is what maps
    /// such a name back onto the belt it came out of.
    std::vector<std::string> rounds{};
    translate::translate_index_t name_index{}; ///< what the game calls it, invalid when it names it nowhere
  };

  struct Ammunition {
    int count{};
    std::string name{};
  };

  struct TurretData {
    Point3 rel;
    Point3 abs;
    Point3 gun_rel;
    Point3 gun_abs;

    bool operator==(const TurretData &other) const {
      return rel == other.rel && abs == other.abs && gun_rel == other.gun_rel && gun_abs == other.gun_abs;
    }
  };

  struct TurretDesc {
    TurretNode *head;
    TurretNode *gun;
    ObjectRewindState<TurretData, false, false, false> turret_state{};

    void setData(Point3 turret_info) {
      head->turret_abs = {};
      head->turret_rel = turret_info;
      head->turret_rel.y = 0.f;
      gun->turret_rel.y = turret_info.y;
    }

    void push_curr_state(ParserState &state);
  };

  struct Weapon {
    int weapon_id = -1;
    int weapon_index = -1;
    std::string emitter{};
    std::string blk_path{};
    std::string weapon_name{};
    translate::translate_index_t name_index{}; // weapon name translate index
    translate::translate_index_t name_index_short{};
    std::vector<Ammunition> munitions{}; // not currently populated
    /// Came from the WeaponPilons block rather than from commonWeapons or a preset.
    /// See Unit::Load for what that block is and why it needs its own pass.
    bool from_pilon = false;
    std::unique_ptr<TurretDesc> turret_desc{};

    Weapon(const DataBlock *blk, Unit *unit, std::vector<uint16_t> &weapons_count);

    /// What a loadout can put in this gun: the stock load first, then the named sets in
    /// blk order. The pick itself is not here:
    /// it is the Unit::storage_weapons line naming one of these by BulletSet::name.
    const std::vector<BulletSet> &bulletSets() const;

    void loadTurretData(const DataBlock *weapon_blk, TurretTree *tree);
  };

  class Unit {
  protected:
    ParserState *state = nullptr;
    void loadWeaponData(const std::string &path);
    void loadTurretData(Weapon &weapon, const DataBlock *weap_blk);

    bool has_tree = false;
    GeomNodeTree geom_tree{};
    std::unique_ptr<TurretTree> turret_tree{};

    friend Weapon;

  public:
    /// Every node of the <model>_dm_skeleton, in tree order. The part ids of HitOutcome
    /// number the damage model parts, which live in here; which nodes of the tree the
    /// server counts is for the caller to decide, so the list goes out as it is.
    std::vector<std::string> damage_parts{};

    bool hasTree() const { return has_tree; }

    bool LoadFromStorage(const FieldSerializerDict &dict);

    virtual ~Unit() = default;
    virtual void Load();
    virtual const char *getUnitTypeName() = 0;

    Weapon *getWeapon(uint16_t idx);
    Weapon *getWeaponFromRef(uint32_t ref);
    void calculateTurretData();

    std::vector<std::string> getTags() const { return getUnitTagsBlk(unit_tags); }

    Unit(ParserState *state, uint16_t uid, UnitType unit_type) : state(state), uid(uid), unitType(unit_type) {}

    BaseExtReflectable *base_data = nullptr;
    DVMReflectable *base_dvm_data = nullptr;
    uint32_t created_at_ms = 0;
    uint32_t killed_at_ms = 0xFFFFFFFF; // when was killed
    Point3 killed_position{}; // only valid when killed_at_ms is set
    uint32_t destroyed_at_ms = 0xFFFFFFFF; // when was 'destroyed' in ecs
    uint16_t uid;
    ecs::EntityId curr_eid;
    UnitType unitType; // make into an enum, maybe match with gaijin enum? I know they have one iirc
    std::string raw_unit_name{}; // as seen in data
    std::string unit_name{}; // unit name with tankModel/ prefix removed
    translate::translate_index_t name_index_shop{}; // name used in shop
    translate::translate_index_t name_index_0{}; // type 0
    translate::translate_index_t name_index_1{}; // short type 1,
    translate::translate_index_t name_index_2{}; // short version 2, for tanks this is their unit type
    std::string player_internal_name{};
    int owner_pid{};
    TMatrix spawn_position{};
    std::string loadout_name{};
    std::string skin_name{};
    DataBlock camo_info{};
    DataBlock custom_weapons_blk{};
    std::vector<weapon_data> storage_weapons{};
    std::vector<std::string> weapon_mods{};
    std::vector<std::string> fm_mods{};
    std::vector<Weapon> weapons{};
    ObjectRewindState<SpaceTimeEuler, false, false, false> positions{};

    UnitWeaponsMask weapons_mask{state};

    const DataBlock *unit_wpcost{};
    const DataBlock *unit_tags{};

    Tank *AsTank();

    Aircraft *AsAircraft();
  };


  bool LoadFromStorage(Unit *unit, const FieldSerializerDict &data);

  class Aircraft : public Unit {
  private:
    FMWReflectable fmv_data{state};
    FM_DVMReflectable fm_dvm_data{state};

  public:
    const char *getUnitTypeName() override;
    explicit Aircraft(ParserState *state, uint16_t uid) : Unit(state, uid, AircraftType) {
      base_data = &fmv_data;
      base_dvm_data = &fm_dvm_data;
      fmv_data.owner_unit = this;
    }

    ~Aircraft() override = default;
    void Load() override;
  };

  class Tank : public Unit {
    GMReflectable gm_data{state};
    GM_DVMReflectable gm_dvm_data{state};


  public:
    const char *getUnitTypeName() override;
    void Load() override;

    explicit Tank(ParserState *state, uint16_t uid) : Unit(state, uid, TankType) {
      base_data = &gm_data;
      base_dvm_data = &gm_dvm_data;
      gm_data.owner_unit = this;
    }

    ~Tank() override = default;
  };

  struct UnitRef {
    Unit *unit = nullptr;
    bool operator==(const UnitRef &other) const { return unit == other.unit; }
  };
} // namespace unit
template<>
struct fmt::formatter<UnitType> {
public:
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
  template<typename Context>
  constexpr auto format(UnitType const &val, Context &ctx) const {
    const char *str = nullptr;
    switch (val) {
      case UnitType::TankType: str = "TankType"; break;
      case UnitType::AircraftType: str = "AircraftType"; break;
      default: str = "UNKNOWN"; break;
    }
    return format_to(ctx.out(), "{}({})", static_cast<int>(val), str);
  }
};
