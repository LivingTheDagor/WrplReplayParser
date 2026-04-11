#pragma once

#include "vector"
#include "cstdint"
#include "danet/BitStream.h"
#include "math/dag_Point3.h"

struct CameraTime {
  uint32_t time_ms;
  // todo, I know the structure just have to reimplement it in cpp
};

struct SpaceTime {
  uint32_t time_ms=0;
  Point3 location{};

  bool operator==(const SpaceTime& other) const {
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

enum UnitType: uint8_t {
  TankType = 1,
  AircraftType = 2,

};

class FieldSerializerDict;

namespace unit {

  class Aircraft;
  class Tank;


  class Unit {
  public:
    virtual ~Unit() = default;
    Unit(uint16_t uid, UnitType unit_type) : uid(uid), unitType(unit_type) {}
    uint16_t uid;
    UnitType unitType; // make into an enum, maybe match with gaijin enum?
    std::vector<CameraTime> camera_pos;
    Tank * AsTank();
    Aircraft * AsAircraft();
  };


  bool LoadFromStorage(Unit * unit, FieldSerializerDict * data);

  class Aircraft : public Unit {
  public:
    Aircraft(uint16_t uid) : Unit(uid, AircraftType) {}
    virtual ~Aircraft() = default;
    std::vector<SpaceTime> positions{};
  };

  class Tank: public Unit {
  public:
    virtual ~Tank() = default;

  };

  class UnitRef {
  public:
    Unit * unit= nullptr;
    ~UnitRef() {
      delete unit;
    }
  };
}