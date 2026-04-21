#pragma once

#include "vector"
#include "cstdint"
#include "danet/BitStream.h"
#include "math/dag_Point3.h"
#include "DataBlock.h"
//#include "mpi/mpi.h"
//#include "mpi/codegen/ReflIncludes.h"

struct CameraTime {
  uint32_t time_ms;
  // todo, I know the structure just have to reimplement it in cpp
};

struct SpaceTime {
  uint32_t time_ms = 0;
  Point3 location{};

  bool operator==(const SpaceTime &other) const {
    return time_ms == other.time_ms && location == other.location;
  }
};


struct SensorsControlStates {
  // a whole bunch of this is probably a union actually
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

  bool deserialize(BitStream *bs);
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

  bool deserialize(BitStream *bs);
};

enum UnitType : uint8_t {
  TankType = 1,
  AircraftType = 2,

};

class FieldSerializerDict;

namespace unit {
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

  struct Ammunition{
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
    //std::string ammunition_count{};
  };

  class Unit {
  public:
    virtual ~Unit() = default;
    virtual void Load() {};

    Unit(uint16_t uid, UnitType unit_type) : uid(uid), unitType(unit_type) {}

    uint16_t uid;
    UnitType unitType; // make into an enum, maybe match with gaijin enum? I know they have one iirc
    std::string unit_name{};
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

    Tank *AsTank();

    Aircraft *AsAircraft();
  };


  bool LoadFromStorage(Unit *unit, FieldSerializerDict *data);

  class Aircraft : public Unit {
  public:
    Aircraft(uint16_t uid) : Unit(uid, AircraftType) {}

    virtual ~Aircraft() = default;
    void Load() override;

    std::vector<SpaceTime> positions{};
    Weapon *getWeapon(uint32_t ref);
  };

  class Tank : public Unit {
  public:
    Tank(uint16_t uid) : Unit(uid, TankType) {}
    virtual ~Tank() = default;

  };

  class UnitRef {
  public:
    Unit *unit = nullptr;

    ~UnitRef() {
      delete unit;
    }
  };
}