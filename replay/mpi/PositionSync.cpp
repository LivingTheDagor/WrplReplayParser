#include "state/ParserState.h"
#include "mpi/PositionSync.h"
#include "Unit.h"
#include "danet/dag_netUtils.h"
#include "math/dag_mathAng.h"

bool separateServerSideDetection_g = true;
bool resyncSaclosGuidanceParams = true; // actually known to be true, hopefully

bool SensorsControlStates::deserialize(BitStream &bs) {
  first_bool = bs.ReadBit();
  uint8_t sensor_type;
  bs.Read(sensor_type);
  sensor_type_maybe = sensor_type;
  sensor_type >>= 4;
  switch (sensor_type) {
    case 1: {
      bool bool_2 = bs.ReadBit();
      if (!bool_2)
        return true;
      int16_t bit_packed;
      int16_t packed_val_1;
      int16_t packed_val_2;
      int16_t packed_val_3;
      bs.Read(bit_packed);
      field136_0x88 = (uint8_t) bit_packed & 0xf;
      field137_0x89 = (uint8_t) (bit_packed >> 4) & 0x3f;
      field138_0x8a = (uint8_t) (bit_packed >> 10) & 0xf;
      bs.Read(some_data_1);
      bs.Read(packed_val_1);
      bs.Read(packed_val_2);
      bs.Read(packed_val_3);
      some_data_2 = netutils::UNPACKS<int16_t>(packed_val_1, 1.0f);
      some_data_4 = netutils::UNPACKS<int16_t>(packed_val_2, PI);
      some_data_5 = netutils::UNPACKS<int16_t>(packed_val_2, PI);
      if (bit_packed < 0) {
        bs.Read(some_data_6[0]);
      }
      bool bool_thing = bs.ReadBit();
      if (bool_thing) {
        uint8_t bVar7;
        bs.Read(bVar7);
        this->some_data_6[1] = bVar7 & 1;
        this->field149_0xa4 = (int) ((char) (bVar7 << 5) >> 6);
        this->field150_0xa8 = (int) ((char) (bVar7 << 3) >> 6);
      }
      break;
    }
    case 2: {
      if (first_bool == 0) {
        return true;
      }
      field136_0x88 = bs.ReadBit();
      int16_t v;
      bs.Read(v);
      some_data_1 = netutils::UNPACKS<int16_t>(v, PI);
      bs.ReadBits(reinterpret_cast<uint8_t *>(&some_data_2), 0x60);
      bs.ReadBits(reinterpret_cast<uint8_t *>(&some_data_5),
                  0x60); // ok for some fucking reason some_data_6 is a float here
      bs.Read(field147_0xa8);
      break;
    }
    case 3: {
      G_ASSERT(false); // uglugluglug
      break;
    }
    case 4: {
      bool stuff_is_read = bs.ReadBit();
      if (stuff_is_read) {
        bs.ReadBits(reinterpret_cast<uint8_t *>(&some_data_1), 0x60);
      }
      break;
    }
    default: G_ASSERT(false);
  }
  bool stores_vec_thang = bs.ReadBit();
  if (stores_vec_thang) {
    bs.ReadBits(&field132_0x84, 6);
    field4_0x4.resize(field132_0x84);
    for (int i = 0; i < field132_0x84; i++) {
      bs.Read(field4_0x4[i]);
    }
    bs.ReadBits(&field133_0x85, 6);
  }
  return true;
}


bool TargetDesignationControlState::deserialize(BitStream &bs) {
  bs.Read(v1);
  bs.Read(v2);
  v3 = bs.ReadBit();
  if (v3) {
    bs.Read(v4);
  }
  write_compressed = bs.ReadBit();
  if (write_compressed) {
    bs.Read(v6);
    netutils::read_vector(bs, v9, 4000.0f);
  } else {
    bs.Read(v7);
    bs.Read(v9);
  }
  bs.Read(v5);
  v8 = bs.ReadBit();
  v10 = bs.ReadBit();
  auto read_some_float = bs.ReadBit();
  if (read_some_float) {
    uint8_t temp;
    bs.Read(temp);
    v11 = netutils::UNPACK<uint8_t>(temp, 1);
  }
  v12 = bs.ReadBit();
  bool has_v14 = bs.ReadBit();
  v13 = bs.ReadBit();
  if (has_v14) {
    bs.Read(v14);
  } else
    v14 = 0;
  bool has_v15 = bs.ReadBit();
  if (has_v15) {
    bs.Read(v15);
  }
  return true;
}

bool CounterMeasuresControlState::deserialize(BitStream &bs) {
  bool ret = true;
  ret &= bs.Read(v1);
  ret &= bs.Read(v2);
  return ret;
}

G_STATIC_ASSERT(sizeof(TargetDesignationControlState) == 0x50);


void DeserializeSeekerData(BitStream &bs) {
  // this will probably never be implemented unless if I actually go through and try to understand what this complex
  // bullshit is
  uint32_t val;
  bs.Read(val);
  auto read_offs = bs.GetReadOffset();
  bs.SetReadOffset(read_offs + val);
  // bool does_read_call_one = bs.ReadBit();

  return;
}

bool FMSync(ParserState &state, BitStream &bs) {
  static int curr_aircraft_index = 0;
  uint16_t uid = 0;
  do {
    bool uid_serialized = bs.ReadBit();
    if (uid_serialized) {
      bs.ReadCompressed(uid);
      uid &= 0x7FF;
    } else {
      uid += 1;
    }
    if (uid == 0x7FF)
      break;
    bool has_data = bs.ReadBit();
    if (!has_data) {
      auto str = fmt::format("uid: {:#x}", uid);
      // LOG("yes to data");
      if (separateServerSideDetection_g == false) {
        G_ASSERT(false); // be ready for if this ever happens
      } else {
        bool lVar30lessThan0 = bs.ReadBit();
        bool cVar4NE0 = bs.ReadBit();
        // if both true, then we do nothing
        if (!(lVar30lessThan0 && cVar4NE0)) {
          auto ref = state.getUnitObj(uid);
          if (!ref)
            return false;
          auto unit = ref->AsAircraft();
          if (!unit)
            return false;
          bool fVar41LessThan0p5 = bs.ReadBit();
          uint32_t some_uint;
          bs.Read(some_uint);
          bool _0x7330_val = bs.ReadBit();
          bool v1 = bs.ReadBit();
          if (v1) {
            bool v2 = bs.ReadBit();
            bool unk1 = bs.ReadBit();
            bool unk2 = bs.ReadBit();
            uint8_t count = 0;
            bs.ReadBits(&count, 4);
            std::vector<bool> temp_bits{};
            temp_bits.reserve(count);
            for (int i = 0; i < count; i++) {
              temp_bits.push_back(bs.ReadBit());
            }
          }
          int zig_val;
          bs.ReadZigZag(zig_val);
          if (zig_val < 0) { // no negatives
            return false;
          }
          std::vector<int32_t> vals;
          vals.resize(zig_val);
          for (int i = 0; i < zig_val; i++) { // actually is weapons
            int temp, temp1;
            bs.ReadZigZag(temp);
            bs.ReadZigZag(temp1);
            if (temp1 > 0)
              for (int ii = 0; ii < temp1; ii++) {
                int temp_zig;
                bs.ReadZigZag(temp_zig);
              }
            vals[i] = temp;
          }
          uint32_t some_packed_val;
          uint32_t vals_4;
          SpaceTime st{state.curr_time_ms, {}};
          bs.Read(st.location);
          unit->positions.push_back(st);
          bs.Read(some_packed_val);
          bs.Read(vals_4);
          uint64_t
            val_5; // holds lots of important data, more like a char[7] instead of a uint64_t, data is read as bytes
          bs.AlignReadToByteBoundary();
          bs.ReadBits(reinterpret_cast<uint8_t *>(&val_5), 0x38);
          uint8_t number_of_engines;
          bs.Read(number_of_engines);
          if (number_of_engines > 0xf)
            return false; // "FMsync: numEngines >= MAX_AIRCRAFT_MOTORS"
          for (int i = 0; i < number_of_engines; i++) {
            bool is_some_data_serialized = bs.ReadBit();
            uint8_t v;
            bs.Read(v);
            float probably_engine_power = -1;
            float packed_v_2 = -1;
            if (is_some_data_serialized) {
              int16_t temp_packed;
              bs.Read(temp_packed);
              probably_engine_power = netutils::UNPACKS<int16_t>(temp_packed, 1.1f);
            }

            auto is_some_packed_2 = bs.ReadBit();
            if (is_some_packed_2) {
              int8_t v_;
              bs.Read(v_);
              packed_v_2 = netutils::UNPACKS<int8_t>(v_, 1.0);
            }

            uint8_t v10;
            uint8_t v11;
            bs.Read(v10);
            bs.Read(v11);
            float v10f = netutils::UNPACK<uint8_t>(v10, 1.0);
            float v11f = netutils::UNPACK<uint8_t>(v11, 1.0);
          }
          uint8_t sensorsCount;
          bs.Read(sensorsCount);
          G_ASSERT(sensorsCount <= SENSORS_COUNT);
          std::vector<SensorsControlStates> sensors{sensorsCount};
          for (auto &s: sensors) {
            G_ASSERT(s.deserialize(bs));
          }
          uint8_t v;
          if (sensorsCount != 0) {
            bs.Read(v);
          }

          uint8_t counterMeasuresCount;
          bs.Read(counterMeasuresCount);
          G_ASSERT(counterMeasuresCount <= COUNTER_MEASURES_COUNT);
          std::vector<CounterMeasuresControlState> counterMeasures{counterMeasuresCount};
          for (auto &c: counterMeasures) {
            G_ASSERT(c.deserialize(bs));
          }
          uint8_t counter_measures_v;
          if (counterMeasuresCount > 0)
            bs.Read(counter_measures_v);
          uint8_t targetsNum = 0;
          bs.ReadBits(&targetsNum, 4);
          G_ASSERT(targetsNum <= TARGETS_NUM);
          std::vector<TargetDesignationControlState> targets{targetsNum};
          for (auto &t: targets) {
            t.deserialize(bs);
          }
          bool bit_thing;
          bs.Read(bit_thing);
          if (bit_thing) {
            uint16_t vt_1;
            uint16_t vt_2;
            bs.Read(vt_1);
            bs.Read(vt_2);
          }
          bool bit_thing_2;
          bs.Read(bit_thing_2);
          if (bit_thing_2) {
            DeserializeSeekerData(bs);
          }
        }
        curr_aircraft_index++;
      }
    } else {
      // LOG("no aircraft data for {}", uid);
    }
    // G_ASSERT(uid != 0); // WHY THE HELL CAN WE HAVE A UID THAT IS 0
  } while (uid != 0x7FF);
  float maybe_floats[3];
  bs.Read(maybe_floats);
  // some extra data here
  return true;
}

struct TankRef {
  unit::Unit *ref_1 = nullptr; // + 0.0 always matches
  unit::Unit *ref_2 = nullptr; // + 0x8 needs to not match flags 0x10 and 0x1800
  bool val1 = false;
  bool val2 = false;
  bool val3 = false;
  uint8_t turret_count = 0;
};

#if LDAG_DBGLEVEL > 0
#define RET_FAIL(op) G_ASSERT((op));
#else
#define RET_FAIL(op) \
  do {               \
    if (!(op)) {     \
      return false;  \
    }                \
  } while (0)
#endif
struct SubVehicleDynData {
  Quat quat;
  float f4;
  float f5;
  float f6;
  Point3 some_position_maybe;

  bool deserialize(BitStream &bs) {
    Point3 euler;
    std::array<int16_t, 3> euler_bits;
    bool ok = true;
    ok &= bs.Read(euler_bits);
    netutils::unpack_euler_16(euler_bits, euler);
    euler_to_quat(euler.y, euler.z, euler.x, quat);
    quat = normalize(quat);
    int8_t pi_vals[3];
    ok &= bs.Read(pi_vals);
    f4 = netutils::UNPACKS(pi_vals[0], PI);
    f5 = netutils::UNPACKS(pi_vals[0], PI);
    f6 = netutils::UNPACKS(pi_vals[0], PI);
    ok &= bs.Read(some_position_maybe);
    return ok;
  }
};


bool ParseVehicleInfo(ParserState &state, BitStream &bs, TankRef *ref, bool is_from_compress) {
  if (!ref->ref_1) {
    G_ASSERT(is_from_compress);
    return true;
  }
  uint8_t v1;
  RET_FAIL(bs.Read(v1));
  bool b1;
  RET_FAIL(bs.Read(b1));
  if (b1) {
    Point3 direction{};
    if (ref->val1) {
      float floats[4];
      RET_FAIL(bs.Read(floats)); // 4 float reads, then 1 later (usePositionHistoryForSlaves is true
      if (true) {
        if (bs.ReadBit()) {
          float f1;
          float f2[6];
          float f3[4];
          float f4[6];
          float f5[4];
          RET_FAIL(bs.Read(f1));
          RET_FAIL(bs.Read(f2));
          RET_FAIL(bs.Read(f3));
          RET_FAIL(bs.Read(f4));
          RET_FAIL(bs.Read(f5));
        }
      } else {
        float f6;
        bs.Read(f6);
      }
    } else {
      std::array<int16_t, 3> vals;
      RET_FAIL(bs.Read(vals));
      netutils::unpack_euler_16(vals, direction);
    }
    bool bool2, bool3;
    bs.Read(bool2);
    bs.Read(bool3);
    Point3 unit_position{};
    if (bool3) {
      bs.Read(unit_position);
    } else {
      uint32_t tv1, tv2;
      RET_FAIL(bs.Read(tv1));
      RET_FAIL(bs.Read(tv2));
      auto fVar31 = (float) (tv1 >> 10) * 4.7683727E-7f + -1.0f;
      auto fv1 = -1.0f;
      if (-1.0f <= fVar31)
        fv1 = fVar31;
      if (1.0f <= fv1)
        fv1 = 1.0;
      fv1 = fv1 * 36000.0f;
      unit_position.y =
        (float) ((tv1 & 0x3ff) << 10 | tv2 >> 0x16) * 0.007629402f + -100.f; // gets added with some altitude???
      fVar31 = (float) (tv2 & 0x3fffff) * 4.7683727E-7f + -1.0f;
      auto fv3 = -1.0f;
      if (-1.0f <= fVar31)
        fv3 = fVar31;
      if (1.0f <= fv3)
        fv3 = 1.0;
      fv3 *= 36000.0f;
      unit_position.x = fv1;
      unit_position.z = fv3;
    }
    if (ref->ref_1 && ref->ref_1 && ref->ref_1->AsTank()) {
      auto tank = ref->ref_1->AsTank();
      tank->positions.push_back({state.curr_time_ms, unit_position});
    }
    Point3 out_2;
    if (ref->val1) {
      RET_FAIL(bs.Read(out_2));
    } else {
      int16_t temp[3];
      RET_FAIL(bs.Read(temp));
      out_2.x = netutils::UNPACKS(temp[0], 100.0);
      out_2.y = netutils::UNPACKS(temp[1], 100.0);
      out_2.z = netutils::UNPACKS(temp[2], 100.0);
    }
    int8_t temp_vals[3];
    Point3 temp_p3;
    RET_FAIL(bs.Read(temp_vals));
    temp_p3.x = netutils::UNPACKS(temp_vals[0], PI);
    temp_p3.y = netutils::UNPACKS(temp_vals[1], PI);
    temp_p3.z = netutils::UNPACKS(temp_vals[2], PI);
    bool bool4;
    RET_FAIL(bs.Read(bool4));
    uint8_t temp_vals_womp[4];
    if (bool4) {
      bs.Read(temp_vals_womp);
    } else {
      // some mathy logic here
    }
    int8_t local_584 = 0x55; // dunno why??? lmao its what ghidra says the def value is
    RET_FAIL(bs.Read(local_584)); // if bool4 is true, this should be 0
    float local_584_val = netutils::UNPACKS(local_584, 0.5);
    bool bool5;
    RET_FAIL(bs.Read(bool5));
    if (bool5) {
      uint8_t some_vals[6]; // 7 u8 reads
      uint8_t some_vals_2[2]; // 2 u8 reads
      bool some_val_3;
      uint8_t some_val_4[4];
      RET_FAIL(bs.Read(some_vals));
      if (bs.ReadBit()) {
        RET_FAIL(bs.Read(some_vals_2));
      }
      RET_FAIL(bs.Read(some_val_3));
      RET_FAIL(bs.Read(some_val_4));
      G_ASSERT(some_val_4[3] <= 9);
      std::vector<uint8_t> some_vals_5{};
      some_vals_5.resize(some_val_4[3]);
      for (auto &b: some_val_4) {
        RET_FAIL(bs.Read(b));
      }
    }
    bool bool6, bool7, bool8;
    RET_FAIL(bs.Read(bool6));
    RET_FAIL(bs.Read(bool7));
    RET_FAIL(bs.Read(bool8));
    if (bool8) {
      uint8_t some_vals[9];
      bool many_bools[7];
      RET_FAIL(bs.Read(some_vals[0]));
      RET_FAIL(bs.Read(many_bools[0]));
      RET_FAIL(bs.Read(many_bools[1]));
      RET_FAIL(bs.Read(many_bools[2]));
      RET_FAIL(bs.Read(many_bools[3]));
      RET_FAIL(bs.Read(many_bools[4]));
      RET_FAIL(bs.Read(many_bools[5]));
      RET_FAIL(bs.Read(many_bools[6]));

      RET_FAIL(bs.Read(some_vals[1]));
      RET_FAIL(bs.Read(some_vals[2]));
      RET_FAIL(bs.ReadBits(&(some_vals[3]), 5));
      RET_FAIL(bs.ReadBits(&(some_vals[4]), 4));
      RET_FAIL(bs.ReadBits(&(some_vals[5]), 4));
      RET_FAIL(bs.ReadBits(&(some_vals[6]), 2));
      RET_FAIL(bs.ReadBits(&(some_vals[7]), 3));
      RET_FAIL(bs.ReadBits(&(some_vals[8]), 4));
    }

    if (bs.ReadBit()) {
      uint8_t sub_vehicle_count{};
      RET_FAIL(bs.Read(sub_vehicle_count));
      RET_FAIL(sub_vehicle_count <= 4);
      std::vector<SubVehicleDynData> dyn_data{};
      dyn_data.resize(sub_vehicle_count);
      for (auto &dyn: dyn_data) {
        RET_FAIL(dyn.deserialize(bs));
      }
    }
  }
  bool b2;
  bool b3;
  RET_FAIL(bs.Read(b2));
  RET_FAIL(bs.Read(b3));
  for (int i = 0; i < ref->turret_count; i++) {
    bool bool1, bool2, bool3, bool4;
    uint8_t val1, val2;
    RET_FAIL(bs.Read(bool1));
    RET_FAIL(bs.Read(val1));
    RET_FAIL(bs.Read(bool2));
    if (bool2) {
      RET_FAIL(bs.Read(val2));
      double val = netutils::UNPACK(val2, 1.0);
    }
    RET_FAIL(bs.Read(bool3));
    RET_FAIL(bs.Read(bool4));
    if (b2) {
      if (bs.ReadBit()) {
        uint16_t tv1, tv2;
        uint8_t tv18, tv28;
        RET_FAIL(bs.Read(tv1));
        RET_FAIL(bs.Read(tv2));
        RET_FAIL(bs.Read(tv18));
        RET_FAIL(bs.Read(tv28));
        bool extra_stuff;
        RET_FAIL(bs.Read(extra_stuff));
        if (extra_stuff) {
          uint16_t extra_val;
          RET_FAIL(bs.Read(extra_val));
        }
      }
    }
  }
  uint8_t sensorsCount{};
  RET_FAIL(bs.Read(sensorsCount));
  RET_FAIL(sensorsCount <= SENSORS_COUNT);
  std::vector<SensorsControlStates> states{};
  states.resize(sensorsCount);
  for (auto &sensor: states) {
    RET_FAIL(sensor.deserialize(bs));
  }
  if (sensorsCount > 0) {
    uint8_t some_weird_val;
    RET_FAIL(bs.Read(some_weird_val));
  }
  uint8_t counterMeasuresCount;
  bs.Read(counterMeasuresCount);
  RET_FAIL(counterMeasuresCount <= COUNTER_MEASURES_COUNT);
  std::vector<CounterMeasuresControlState> counterMeasures{counterMeasuresCount};
  for (auto &c: counterMeasures) {
    RET_FAIL(c.deserialize(bs));
  }
  uint8_t targetsNum = 0;
  bs.ReadBits(&targetsNum, 4);
  G_ASSERT(targetsNum <= 8);
  std::vector<TargetDesignationControlState> targets{targetsNum};
  for (auto &t: targets) {
    RET_FAIL(t.deserialize(bs));
  }
  return true;
}


bool GMSync(ParserState &state, BitStream &bs) {
  uint16_t uid_lower;
  uint16_t uid_upper;
  float time_at;
  uint16_t packet_reliability_order;
  bs.Read(uid_lower);
  bs.Read(uid_upper);
  uint16_t uid_lower_base = uid_lower;
  bs.Read(time_at);
  bs.Read(packet_reliability_order);
  for (; uid_lower < uid_upper; uid_lower++) {
    TankRef ref{};
    bool bool1;
    RET_FAIL(bs.Read(bool1));
    if (bool1) {
      auto curr_unit = state.getUnitEid(uid_lower);
      // G_ASSERT(curr_unit); // this can apparently fucking happen
      ref.ref_1 = ref.ref_2 = state.getUnitObj(uid_lower);
      // std::string_view name = *state.g_entity_mgr.getNullable<ecs::string>(curr_unit, ECS_HASH("unit__className"));
      bool bool2;
      uint8_t some_val_idk;
      bool bool4;
      RET_FAIL(bs.Read(bool2)); // some weird flag setting based on bool value at unit + 0x90
      if (bs.ReadBit()) {
        RET_FAIL(bs.ReadBits(&some_val_idk, 4));
      }
      RET_FAIL(bs.Read(ref.val2));
      RET_FAIL(bs.Read(ref.val3));
      if (!ref.val2 || !ref.val3) {
        bool bool5;
        bool isCompressed;
        RET_FAIL(bs.Read(bool5));
        RET_FAIL(bs.Read(ref.val1));
        RET_FAIL(bs.Read(ref.turret_count));
        RET_FAIL(bs.Read(isCompressed));
        if (isCompressed) {
          auto &hist = state.NetDelta.getHistory(uid_lower);
          auto res = state.NetDelta.netDelta.readDelta(bs, &hist);
          if (res.ok) {
            ParseVehicleInfo(state, res.bs, &ref, true);
          }
        } else {
          bs.AlignReadToByteBoundary();
          ParseVehicleInfo(state, bs, &ref, false);
          G_ASSERT(false);
        }
        bool bool7;
        RET_FAIL(bs.Read(bool7));
        if (bool7) {
          Point4 tp4; // actually 4 float reads, but meght
          bs.Read(tp4);
        }
        bool bool8;
        RET_FAIL(bs.Read(bool8));
        if (bool8 && ref.turret_count > 0) { // maybe has_shot_gun???
          for (int i = 0; i < ref.turret_count; i++) {
            bool t1;
            RET_FAIL(bs.Read(t1));
            if (t1) {
              uint8_t v1;
              float v2;
              float v3;
              uint8_t v4;
              RET_FAIL(bs.Read(v1));
              RET_FAIL(bs.Read(v2));
              RET_FAIL(bs.Read(v3));
              RET_FAIL(bs.Read(v4));
              // new ammunition counts!!!!
              // first val is ammo 0, second is ammo 1, ...
              for (int j = 0; j < v4; j++) {
                uint16_t vv;
                bs.Read(vv);
              }
            }
          }
        }
        bool has_sensor_info;
        bool has_sensor_info_2;
        bs.Read(has_sensor_info);
        if (has_sensor_info) {
          bs.Read(has_sensor_info_2);
          if (has_sensor_info_2) {
            DeserializeSeekerData(bs);
          }
        }
        bs.AlignReadToByteBoundary();
      }
    }
  }
  return true;
}

Rocket *getRocket(ParserState &state, ecs::EntityId eid) {
  return state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("rocket_component"));
}


Rocket *getBomb(ParserState &state, ecs::EntityId eid) {
  return state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("bomb_component"));
}


Rocket *getTorpedo(ParserState &state, ecs::EntityId eid) {
  return state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("torpedo_component"));
}

typedef Rocket *(*get_weapon_cb)(ParserState &state, ecs::EntityId eid);

bool ParseWeapon(ParserState &state, const BitStream &bs, get_weapon_cb cb) {

  bool bool1, bool2;
  RET_FAIL(bs.Read(bool1));
  RET_FAIL(bs.Read(bool2));
  ecs::EntityId eid;
  RET_FAIL(bs.Read(eid));
  Rocket *entity = cb(state, eid);
  RET_FAIL(entity);
  Point3 WeaponPos{};
  bool is_not_compressed;
  RET_FAIL(bs.Read(is_not_compressed));
  if (is_not_compressed) {
    bs.Read(WeaponPos);
  } else {
    uint32_t comp_1, comp_2;
    RET_FAIL(bs.Read(comp_1));
    RET_FAIL(bs.Read(comp_2));
    float fVar31 = (float) (comp_1 >> 10) * 4.7683727E-7f + -1.0f;
    float v1 = -1.0;
    if (-1.0f <= fVar31)
      v1 = fVar31;
    if (1.0f <= v1)
      v1 = 1.0f;
    v1 = v1 * 15000;

    fVar31 = (float) ((comp_1 & 0x3ff) << 10 | (comp_2 >> 0x16)) * 0.0000019073505f + -1.0f;
    float v2 = -1.0;
    if (-1.0f <= fVar31)
      v2 = fVar31;
    if (1.0f <= v2)
      v2 = 1.0f;
    v2 = v2 * 15000;
    fVar31 = (float) (comp_2 & 0x3fffff) * 4.7683727E-7f + -1.0f;
    float v3 = -1.0;
    if (-1.0f <= fVar31)
      v3 = fVar31;
    if (1.0f <= v3)
      v3 = 1.0f;
    v3 = v3 * 15000;
    WeaponPos = {v1, v2, v3};
  }
  entity->positions.push_back({state.curr_time_ms, WeaponPos});
  if (!bool2) {
    int b5;
    bs.ReadZigZag(b5);
    Point3 b6;
    RET_FAIL(bs.Read(b6));
    Point3 b7;
    Point3 b8;
    uint16_t temp_t;
    RET_FAIL(bs.Read(temp_t));
    b7.x = netutils::UNPACK(temp_t, 4000.0f);
    RET_FAIL(bs.Read(temp_t));
    b7.y = netutils::UNPACK(temp_t, 4000.0f);
    RET_FAIL(bs.Read(temp_t));
    b7.z = netutils::UNPACK(temp_t, 4000.0f);


    RET_FAIL(bs.Read(temp_t));
    b8.x = netutils::UNPACK(temp_t, 30.0f);
    RET_FAIL(bs.Read(temp_t));
    b8.y = netutils::UNPACK(temp_t, 30.0f);
    RET_FAIL(bs.Read(temp_t));
    b8.z = netutils::UNPACK(temp_t, 30.0f);

    bool some_b;
    uint16_t some_val = 0;
    RET_FAIL(bs.Read(some_b));
    if (some_b) {
      RET_FAIL(bs.ReadCompressed(some_val));
    }
    uint8_t flags{};
    RET_FAIL(bs.Read(flags));
    float sf1 = 0, sf2 = 0, sf3 = 0, sf4 = 0;
    uint32_t temp;
    if (flags & 1) {
      RET_FAIL(bs.ReadCompressed(temp));
      sf1 = (float) (temp - 1) / 48.f;
    }
    if (flags & 2) {
      RET_FAIL(bs.ReadCompressed(temp));
      sf2 = (float) (temp - 1) / 48.f;
    }
    if (flags & 4) {
      RET_FAIL(bs.ReadCompressed(temp));
      sf3 = (float) (temp - 1) / 48.f;
    }
    if (flags & 8) {
      RET_FAIL(bs.ReadCompressed(temp));
      sf4 = (float) (temp - 1) / 48.f;
    }
    if (!bool1) {
      float vv;
      RET_FAIL(bs.Read(vv));
      bool vv2;
      RET_FAIL(bs.Read(vv2));
      uint16_t sz;
      RET_FAIL(bs.Read(sz));
      bs.IgnoreBits(sz); // most likely seeker information
    } else if (resyncSaclosGuidanceParams) {
      bool do_stuff;
      RET_FAIL(bs.Read(do_stuff));
      if (do_stuff) {
        uint8_t pack_f1, pack_f2, pack_f3;
        RET_FAIL(bs.Read(pack_f1));
        RET_FAIL(bs.Read(pack_f2));
        RET_FAIL(bs.Read(pack_f3));
        uint8_t some_v3[3];
        RET_FAIL(bs.Read(some_v3));
        bool some_bool;
        RET_FAIL(bs.Read(some_bool));
        if (some_bool) {
          uint8_t pack_ff1, pack_ff2, pack_ff3;
          RET_FAIL(bs.Read(pack_ff1));
          RET_FAIL(bs.Read(pack_ff2));
          RET_FAIL(bs.Read(pack_ff3));
        }
        bool some_bool2;
        RET_FAIL(bs.Read(some_bool2));
        if (some_bool2) {
          uint8_t pack_ff4, pack_ff5, pack_ff6;
          RET_FAIL(bs.Read(pack_ff4));
          RET_FAIL(bs.Read(pack_ff5));
          RET_FAIL(bs.Read(pack_ff6));
        }
      }
    }
  }
  return true;
}


bool WeaponSync(ParserState &state, BitStream &bs) {
  uint8_t rocket_count;
  RET_FAIL(bs.Read(rocket_count));
  for (int i = 0; i < rocket_count; i++) {
    RET_FAIL(ParseWeapon(state, bs, getRocket));
  }

  uint8_t bomb_count;
  RET_FAIL(bs.Read(bomb_count));
  for (int i = 0; i < bomb_count; i++) {
    RET_FAIL(ParseWeapon(state, bs, getBomb));
  }

  uint8_t torpedo_count;
  RET_FAIL(bs.Read(torpedo_count));
  for (int i = 0; i < torpedo_count; i++) {
    RET_FAIL(ParseWeapon(state, bs, getTorpedo));
  }
  return true;
}
