#pragma once

#include "vector"
#include "cstdint"
#include "translate.h"
#include "danet/BitStream.h"
#include "math/dag_Point3.h"
#include <ioSys/dag_dataBlock.h>
#include "ecs/entityId.h"
#include "math/dag_TMatrix.h"
#include "mpi/types.h"
#include "mpi/codegen/ReflIncludes.h"
#include "state/StateRewinder.h"
// #include "mpi/mpi.h"
// #include "mpi/codegen/ReflIncludes.h"

struct CameraTime {
  uint32_t time_ms;
  // todo, I know the structure just have to reimplement it in cpp
};

struct SpaceTime {
  Point3 location{};

  bool operator==(const SpaceTime &other) const { return location == other.location; }
};

struct SpaceTimeEuler : SpaceTime {
  Point3 euler{};

  bool operator==(const SpaceTimeEuler &other) const {
    return SpaceTime::operator==(other) && euler == other.euler;
  }
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

  enum MessageEnum {
    TankPosition = 0xf0a3,
    CameraAngles = 0xf0cc,
  };

  /*class TankPositionMessage : public mpi::Message {
    TankPositionMessage(mpi::IObject *o) : mpi::Message(o, TankPosition) {}
    bool readPayload(ParserState *state) override;
  };

  class UnitCameraMessage : public mpi::Message {
    UnitCameraMessage(mpi::IObject *o) : mpi::Message(o, CameraAngles) {}
    bool readPayload(ParserState *state) override;
  };

  class UnitReflectable : public danet::ReflectableObject {
    mpi::Message *dispatchMpiMessage(mpi::MessageID mid) override;

    void applyMpiMessage(const mpi::Message *) override;
  };*/

  class Aircraft;

  class Tank;

  struct weapon_data {
    std::string launcher{};
    std::string bullet{};
    uint16_t count;
  };

  struct Ammunition {
    int count{};
    std::string name{};
  };

  struct Weapon {
    int weapon_id = -1;
    int weapon_index = -1;
    std::string emitter{};
    std::string blk_path{};
    std::string weapon_name{};
    std::vector<Ammunition> munitions{};
    // std::string ammunition_count{};
  };

  class Unit {
  protected:
  public:
    bool LoadFromStorage(const FieldSerializerDict &dict);

    virtual ~Unit() = default;
    virtual void Load();
    virtual const char * getUnitTypeName() = 0;

    std::vector<std::string> getTags() const { return getUnitTagsBlk(unit_tags); }

    Unit(uint16_t uid, UnitType unit_type) : uid(uid), unitType(unit_type) {}

    // does this entity actually exist in the ECS, or has it been moved after server ordered destruction?
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
    std::vector<CameraTime> camera_pos;
    std::vector<Weapon> weapons{};
    ObjectRewindState<SpaceTimeEuler, false> positions{};

    UnitWeaponsMask weapons_mask{};

    const DataBlock *unit_wpcost{};
    const DataBlock *unit_tags{};

    Tank *AsTank();

    Aircraft *AsAircraft();
  };


  bool LoadFromStorage(Unit *unit, const FieldSerializerDict &data);

  class Aircraft : public Unit {
  private:
    FMWReflectable fmv_data{};
    FM_DVMReflectable fm_dvm_data{};

  public:
    const char *getUnitTypeName() override;
    explicit Aircraft(uint16_t uid) : Unit(uid, AircraftType) {
      base_data = &fmv_data;
      base_dvm_data = &fm_dvm_data;
    }

    ~Aircraft() override = default;
    void Load() override;

    Weapon *getWeapon(uint32_t ref);
  };

  class Tank : public Unit {
    GMReflectable gm_data{};
    GM_DVMReflectable gm_dvm_data{};


  public:
    const char *getUnitTypeName() override;
    void Load() override;

    explicit Tank(uint16_t uid) : Unit(uid, TankType) {
      base_data = &gm_data;
      base_dvm_data = &gm_dvm_data;
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
    const char * str = nullptr;
    switch (val){
      case UnitType::TankType: str = "TankType"; break;
      case UnitType::AircraftType: str = "AircraftType"; break;
      default: str = "UNKNOWN"; break;
    }
    return format_to(ctx.out(), "{}({})", static_cast<int>(val), str);
  }
};
