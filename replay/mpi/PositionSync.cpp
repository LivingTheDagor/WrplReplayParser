#include "state/ParserState.h"
#include "mpi/PositionSync.h"
#include "Unit.h"
#include "daNet/dag_netUtils.h"
#include "math/dag_mathAng.h"
#include <algorithm>
#include <cstring>

bool separateServerSideDetection_g = true;
bool resyncSaclosGuidanceParams = true; // actually known to be true, hopefully

#define RET_FAIL(op) G_ASSERT_RETURN((op), false);
bool SensorsControlStates::deserialize(BitStream &bs) {
  RET_FAIL(bs.Read(first_bool));
  uint8_t sensor_type;
  RET_FAIL(bs.Read(sensor_type));
  sensor_type_maybe = sensor_type;
  sensor_type >>= 4;
  switch (sensor_type) {
    case 1: {
      RET_FAIL(bs.Read(has_state));
      if (!has_state)
        return true;
      int16_t bit_packed;
      int16_t packed_val_1;
      int16_t packed_val_2;
      int16_t packed_val_3;
      RET_FAIL(bs.Read(bit_packed));
      field136_0x88 = (uint8_t) bit_packed & 0xf;
      field137_0x89 = (uint8_t) (bit_packed >> 4) & 0x3f;
      field138_0x8a = (uint8_t) (bit_packed >> 10) & 0xf;
      RET_FAIL(bs.Read(unknown_field_1));
      RET_FAIL(bs.Read(packed_val_1));
      RET_FAIL(bs.Read(packed_val_2));
      RET_FAIL(bs.Read(packed_val_3));
      unknown_field_2 = netutils::UNPACKS<int16_t>(packed_val_1, 1.0f);
      unknown_field_4 = netutils::UNPACKS<int16_t>(packed_val_2, PI);
      unknown_field_5 = netutils::UNPACKS<int16_t>(packed_val_3, PI);
      if (bit_packed < 0) {
        has_unknown_field_6_byte0 = true;
        RET_FAIL(bs.Read(unknown_field_6_data[0]));
      }
      bool bool_thing;
      RET_FAIL(bs.Read(bool_thing));
      if (bool_thing) {
        has_unknown_fields_149_150 = true;
        uint8_t bVar7;
        RET_FAIL(bs.Read(bVar7));
        this->unknown_field_6_data[1] = bVar7 & 1;
        const int field149 = (bVar7 >> 1) & 3;
        const int field150 = (bVar7 >> 3) & 3;
        this->unknown_field_149 = field149 < 2 ? field149 : field149 - 4;
        this->unknown_field_150 = field150 < 2 ? field150 : field150 - 4;
      }
      break;
    }
    case 2: {
      if (first_bool == 0) {
        return true;
      }
      bool flag;
      RET_FAIL(bs.Read(flag));
      field136_0x88 = flag;
      int16_t v;
      RET_FAIL(bs.Read(v));
      unknown_field_1 = netutils::UNPACKS<int16_t>(v, PI);
      RET_FAIL(bs.Read(unknown_field_2));
      RET_FAIL(bs.Read(unknown_field_3));
      RET_FAIL(bs.Read(unknown_field_4));
      RET_FAIL(bs.Read(unknown_field_5));
      RET_FAIL(bs.ReadBits(reinterpret_cast<uint8_t *>(unknown_field_6_data), 32));
      RET_FAIL(bs.Read(unknown_field_7));
      RET_FAIL(bs.Read(unknown_field_147));
      break;
    }
    case 3: {
      return false; // unsupported sensor layout
    }
    case 4: {
      bool stuff_is_read;
      RET_FAIL(bs.Read(stuff_is_read));
      has_kind4_state = stuff_is_read;
      if (stuff_is_read) {
        RET_FAIL(bs.Read(unknown_field_1));
        RET_FAIL(bs.Read(unknown_field_2));
        RET_FAIL(bs.Read(unknown_field_3));
      }
      break;
    }
    default: RET_FAIL(false);
  }
  bool stores_vec_thang;
  RET_FAIL(bs.Read(stores_vec_thang));
  if (stores_vec_thang) {
    RET_FAIL(bs.ReadBits(&contact_count, 6));
    contacts.resize(contact_count);
    for (int i = 0; i < contact_count; i++) {
      RET_FAIL(bs.Read(contacts[i]));
    }
    RET_FAIL(bs.ReadBits(&contacts_tail, 6));
  }
  return true;
}


bool TargetDesignationControlState::deserialize(BitStream &bs) {
  RET_FAIL(bs.Read(unknown_field_1));
  RET_FAIL(bs.Read(unknown_field_2));
  RET_FAIL(bs.Read(has_unknown_field_4));
  if (has_unknown_field_4) {
    RET_FAIL(bs.Read(unknown_field_4));
  }
  RET_FAIL(bs.Read(uses_compressed_vector));
  if (uses_compressed_vector) {
    RET_FAIL(bs.Read(unknown_field_6));
    RET_FAIL(netutils::read_vector(bs, unknown_field_9, 4000.0f));
  } else {
    RET_FAIL(bs.Read(unknown_field_7));
    RET_FAIL(bs.Read(unknown_field_9));
  }
  RET_FAIL(bs.Read(unknown_field_5));
  RET_FAIL(bs.Read(unknown_field_8));
  bool field_10;
  RET_FAIL(bs.Read(field_10));
  unknown_field_10 = field_10;
  RET_FAIL(bs.Read(has_unknown_field_11));
  if (has_unknown_field_11) {
    uint8_t temp;
    RET_FAIL(bs.Read(temp));
    unknown_field_11 = netutils::UNPACK<uint8_t>(temp, 1);
  }
  RET_FAIL(bs.Read(unknown_field_12));
  RET_FAIL(bs.Read(has_unknown_field_14));
  RET_FAIL(bs.Read(unknown_field_13));
  if (has_unknown_field_14) {
    RET_FAIL(bs.Read(unknown_field_14));
  } else
    unknown_field_14 = 0;
  RET_FAIL(bs.Read(has_unknown_field_15));
  if (has_unknown_field_15) {
    RET_FAIL(bs.Read(unknown_field_15));
  }
  return true;
}

bool CounterMeasuresControlState::deserialize(BitStream &bs) {
  RET_FAIL(bs.Read(v1));
  RET_FAIL(bs.Read(v2));
  return true;
}
std::vector<std::string> unit::getUnitTagsBlk(const DataBlock *blk) {
  if (!blk)
    return {};
  std::vector<std::string> tags{};
  if (blk->isEmpty())
    return tags;
  auto tags_blk = blk->getBlockByNameEx("tags");
  for (uint32_t i = 0; i < tags_blk->paramCount(); i++) {
    auto name = tags_blk->getParamName(i);
    if (name)
      tags.push_back(name);
  }
  return tags;
}
std::vector<std::string> unit::getUnitTagsName(std::string_view &name) {
  auto blk = ecs::g_ecs_data->unit_tags.getBlockByNameEx(name);
  if (!blk)
    return {};
  return getUnitTagsBlk(blk);
}
static bool ReadSeekerBits(ParserState &state, const BitStream &bs, uint32_t bits, mpi::SeekerEvent &ev) {
  RET_FAIL(bits <= bs.GetNumberOfUnreadBits());
  if (!state.keep.seekers || (ev.source == mpi::SeekerAircraft && !ev.unit)) {
    bs.IgnoreBits(bits);
    return true;
  }
  ev.bits = bits;
  ev.data.resize(BITS_TO_BYTES(bits));
  RET_FAIL(bs.ReadBits(ev.data.data(), bits));
  state.SeekerEvents.push_back(std::move(ev));
  return true;
}

// Seeker fields are read MSB first in each byte; a multi-byte number is little-endian
// from its bit offset. The last partial byte of the block is right-aligned.
static uint8_t SeekerByte(const mpi::SeekerEvent &ev, uint32_t at) {
  const uint32_t full = ev.bits / 8 * 8, rem = ev.bits % 8;
  uint8_t out = 0;
  for (uint32_t i = at; i < at + 8; i++) {
    uint8_t bit = i < full ? (ev.data[i / 8] >> (7 - i % 8)) & 1 : (ev.data[i / 8] >> (rem - 1 - (i - full))) & 1;
    out = (uint8_t) (out << 1 | bit);
  }
  return out;
}

template <typename T>
static T SeekerValue(const mpi::SeekerEvent &ev, uint32_t at) {
  uint8_t raw[sizeof(T)];
  for (uint32_t i = 0; i < sizeof(T); i++)
    raw[i] = SeekerByte(ev, at + 8 * i);
  T out;
  memcpy(&out, raw, sizeof(T));
  return out;
}

mpi::SeekerState mpi::DecodeSeeker(const SeekerEvent &ev) {
  SeekerState st{};
  if (ev.data.size() < (uint64_t(ev.bits) + 7) / 8)
    return st;
  if (ev.source == SeekerAircraft) {
    if (ev.bits == 518) {
      // The 518-bit layout contains one seeker state. Its slot's presence bit
      // follows one bit for each preceding absent slot, so the state's fields
      // shift by one bit per slot relative to the second-slot offsets.
      uint32_t slot = 0;
      while (slot < 3 && !(ev.data[slot / 8] & (0x80u >> (slot % 8))))
        ++slot;
      if (slot == 3)
        return st;
      const int offset = int(slot) - 1;
      st.ir_state = (SeekerByte(ev, 43 + offset) >> 5) & 7;
      // -FLT_MAX until the seeker first locks
      float t = SeekerValue<float>(ev, 50 + offset);
      if (t > -1e30f)
        st.lock_time = t;
      st.seeker_dir = Point3(SeekerValue<float>(ev, 149 + offset), SeekerValue<float>(ev, 181 + offset),
                             SeekerValue<float>(ev, 213 + offset));
      st.gimbal_az = SeekerValue<float>(ev, 85 + offset);
      st.gimbal_el = SeekerValue<float>(ev, 117 + offset);
      st.fov_half = SeekerValue<float>(ev, 341 + offset);
    } else if (ev.bits == 895) {
      st.target_pos = Point3(SeekerValue<float>(ev, 83), SeekerValue<float>(ev, 115), SeekerValue<float>(ev, 147));
    }
    return st;
  }
  if (ev.source != SeekerWeapon)
    return st;
  if (ev.bits != 607 && ev.bits != 639 && ev.bits != 283)
    return st;
  uint32_t los_at, extra = 0;
  // Bits 8-10: 111 track, 001 search; 100 in the 639-bit block, sent once as the seeker
  // goes to track, which then has 32 bits of unknown meaning before the 607-bit layout.
  uint8_t mode = (SeekerByte(ev, 8) >> 5) & 7;
  if (ev.bits == 607 || ev.bits == 639) {
    extra = ev.bits == 639 ? 32 : 0;
    if (ev.bits == 607 && (mode == 7 || mode == 1))
      st.tracking = mode == 7;
    else if (ev.bits == 639 && mode == 4)
      st.tracking = true;
    st.target_pos = Point3(SeekerValue<float>(ev, 11), SeekerValue<float>(ev, 43), SeekerValue<float>(ev, 75));
    st.range = (float) SeekerValue<uint16_t>(ev, 454 + extra);
    los_at = 340 + extra;
  } else if (ev.bits == 283) {
    los_at = 58;
  } else {
    return st;
  }
  st.los = Point3(SeekerValue<int16_t>(ev, los_at), SeekerValue<int16_t>(ev, los_at + 16),
                  SeekerValue<int16_t>(ev, los_at + 32)) / 32767.f;
  st.decoded = true;
  return st;
}

bool FMSync(ParserState &state, BitStream &bs) {
  ZoneScoped;
  // Do not publish part of a packet if a later aircraft cannot be decoded.
  struct EventRollback {
    ParserState &state;
    size_t ammo, controls, sensors, designations, seekers;
    bool committed = false;
    ~EventRollback() {
      if (!committed) {
        state.AmmoEvents.resize(ammo);
        state.ControlEvents.resize(controls);
        state.SensorEvents.resize(sensors);
        state.DesignationEvents.resize(designations);
        state.SeekerEvents.resize(seekers);
      }
    }
  } rollback{state, state.AmmoEvents.size(), state.ControlEvents.size(), state.SensorEvents.size(),
             state.DesignationEvents.size(), state.SeekerEvents.size()};
  // Reused per thread to avoid an allocation on each aircraft packet.
  thread_local std::vector<std::pair<unit::Aircraft *, SpaceTimeEuler>> positions;
  positions.clear();
  uint16_t uid = 0;
  do {
    bool uid_serialized;
    RET_FAIL(bs.Read(uid_serialized));
    if (uid_serialized) {
      RET_FAIL(bs.ReadCompressed(uid));
      uid &= 0x7FF;
    } else {
      uid += 1;
    }
    if (uid == 0x7FF)
      break;
    bool has_data;
    RET_FAIL(bs.Read(has_data));
    if (!has_data) {
      // LOG("yes to data");
      if (separateServerSideDetection_g == false) {
        G_ASSERT(false); // be ready for if this ever happens
      } else {
        bool lVar30lessThan0, cVar4NE0;
        RET_FAIL(bs.Read(lVar30lessThan0));
        RET_FAIL(bs.Read(cVar4NE0));
        // if both true, then we do nothing
        if (!(lVar30lessThan0 && cVar4NE0)) {
          // A sync can come before the ECS creates its aircraft. The game does not resolve it either:
          // the record is read so the stream stays aligned, and nothing is stored for it.
          auto ref = state.getUnitObj(uid);
          auto unit = ref ? ref->AsAircraft() : nullptr;
          mpi::ControlEvent control{state.curr_time_ms, unit};
          RET_FAIL(bs.Read(control.on_ground));
          uint32_t some_uint;
          RET_FAIL(bs.Read(some_uint));
          bool _0x7330_val, triggers;
          RET_FAIL(bs.Read(_0x7330_val));
          RET_FAIL(bs.Read(triggers));
          if (triggers) {
            RET_FAIL(bs.Read(control.fire_mg));
            RET_FAIL(bs.Read(control.fire_cannon));
            RET_FAIL(bs.Read(control.fire_additional));
            // The count is the number of gun-type entries in the weapon list: guns, gun pods, turret guns, the
            // dummy optic and HMD turrets and targeting pods (338 of 338 aircraft). The bit for each was 1 in
            // only 19 syncs; its meaning is not known.
            uint8_t count = 0;
            RET_FAIL(bs.ReadBits(&count, 4));
            // std::vector<uint32_t>
            dag::Vector<bool, StateAllocator::DagAllocType> temp_bits(state.getMem());
            temp_bits.reserve(count);
            for (int i = 0; i < count; i++) {
              bool bit;
              RET_FAIL(bs.Read(bit));
              temp_bits.push_back(bit);
            }
          }
          int zig_val = 0;
          RET_FAIL(bs.ReadZigZag(zig_val));
          if (zig_val < 0 || zig_val > MAX_WEAPONS_PER_UNIT) { // no negatives
            return false;
          }
          // Rounds left in each weapon of the aircraft, in the order of actual_weapons.
          // An aircraft sends no trigger events at all, so this counter is the only
          // record of it firing: a step down is that many rounds gone. Checked on a
          // battle where 1181 of 1252 impacts by an aircraft have a step within 200 ms,
          // median 62. It rides every update, so the same value repeats; the drop is
          // what carries meaning, and finding it is left to the reader, as with the
          // ground counter this joins.
          for (int i = 0; i < zig_val; i++) {
            // ReadZigZag leaves the value untouched on failure; reject a short read
            // before it can be published as an ammunition count.
            int rounds_left = -1, extra = 0;
            RET_FAIL(bs.ReadZigZag(rounds_left));
            RET_FAIL(bs.ReadZigZag(extra));
            if (extra > 0)
              for (int ii = 0; ii < extra; ii++) {
                // Value goes nowhere, but a failure here desyncs the bit stream and the
                // reads after it - position, engines - would drift silently.
                int temp_zig = 0;
                RET_FAIL(bs.ReadZigZag(temp_zig));
              }
            if (unit && rounds_left >= 0)
              state.AmmoEvents.push_back(
                {state.curr_time_ms, unit, uint8_t(i), 0, uint32_t(rounds_left)});
          }
          uint32_t some_packed_val;
          uint32_t vals_4;
          SpaceTimeEuler st{{}, {}};
          RET_FAIL(bs.Read(st.location));
          RET_FAIL(bs.Read(some_packed_val));
          Point3 someEuler;
          netutils::unpack_euler(some_packed_val, st.euler);
          if (unit)
            positions.emplace_back(unit, st);

          RET_FAIL(bs.Read(vals_4));
          Point3 vel;
          netutils::unpack_velocity(vals_4, vel, 500);
          RET_FAIL(bs.GetNumberOfUnreadBits() >= ((8 - (bs.GetReadOffset() & 7)) & 7));
          bs.AlignReadToByteBoundary();
          RET_FAIL(bs.ReadBits(control.controls, 0x38));
          uint8_t number_of_engines;
          RET_FAIL(bs.Read(number_of_engines));
          if (number_of_engines > 0xf)
            return false; // "FMsync: numEngines >= MAX_AIRCRAFT_MOTORS"
          control.engines.resize(number_of_engines);
          for (auto &engine: control.engines) {
            bool has_afterburner;
            RET_FAIL(bs.Read(has_afterburner));
            RET_FAIL(bs.Read(engine.state));
            if (has_afterburner) {
              int16_t packed;
              RET_FAIL(bs.Read(packed));
              engine.afterburner = netutils::UNPACKS<int16_t>(packed, 1.1f);
            }
            bool has_health;
            RET_FAIL(bs.Read(has_health));
            if (has_health) {
              int8_t packed;
              RET_FAIL(bs.Read(packed));
              engine.health = netutils::UNPACKS<int8_t>(packed, 1.0);
            }
            RET_FAIL(bs.Read(engine.radiators));
          }
          if (unit && state.keep.controls)
            state.ControlEvents.push_back(std::move(control));
          uint8_t sensorsCount;
          RET_FAIL(bs.Read(sensorsCount));
          RET_FAIL(sensorsCount <= SENSORS_COUNT);
          std::pmr::vector<SensorsControlStates> sensors{sensorsCount, state.get_allocator()};
          for (auto &s: sensors) {
            RET_FAIL(s.deserialize(bs));
          }
          uint8_t v = 0;
          if (sensorsCount != 0) {
            RET_FAIL(bs.Read(v));
          }
          for (uint8_t i = 0; i < sensorsCount; i++)
            if (unit && state.keep.sensors)
              state.SensorEvents.push_back({state.curr_time_ms, unit, i, v, sensors[i]});

          uint8_t counterMeasuresCount;
          RET_FAIL(bs.Read(counterMeasuresCount));
          RET_FAIL(counterMeasuresCount <= COUNTER_MEASURES_COUNT);
          std::pmr::vector<CounterMeasuresControlState> counterMeasures{counterMeasuresCount, state.get_allocator()};
          for (auto &c: counterMeasures) {
            RET_FAIL(c.deserialize(bs));
          }
          uint8_t counter_measures_v;
          if (counterMeasuresCount > 0)
            RET_FAIL(bs.Read(counter_measures_v));
          uint8_t targetsNum = 0;
          RET_FAIL(bs.ReadBits(&targetsNum, 4));
          RET_FAIL(targetsNum <= TARGETS_NUM);
          std::pmr::vector<TargetDesignationControlState> targets{targetsNum, state.get_allocator()};
          for (uint8_t i = 0; i < targetsNum; i++) {
            RET_FAIL(targets[i].deserialize(bs));
            if (unit && state.keep.designations)
              state.DesignationEvents.push_back({state.curr_time_ms, unit, i, targets[i]});
          }
          bool bit_thing;
          RET_FAIL(bs.Read(bit_thing));
          if (bit_thing) {
            uint16_t vt_1;
            uint16_t vt_2;
            RET_FAIL(bs.Read(vt_1));
            RET_FAIL(bs.Read(vt_2));
          }
          bool bit_thing_2;
          RET_FAIL(bs.Read(bit_thing_2));
          if (bit_thing_2) {
            uint32_t bits;
            RET_FAIL(bs.Read(bits));
            mpi::SeekerEvent event{state.curr_time_ms, mpi::SeekerAircraft};
            event.unit = unit;
            RET_FAIL(ReadSeekerBits(state, bs, bits, event));
          }
        }
      }
    } else {
      // LOG("no aircraft data for {}", uid);
    }
    // G_ASSERT(uid != 0); // WHY THE HELL CAN WE HAVE A UID THAT IS 0
  } while (uid != 0x7FF);
  // In a client replay: the position of the author's own aircraft, equal to its sync
  // position. All 0 in a server replay.
  float maybe_floats[3];
  RET_FAIL(bs.Read(maybe_floats));
  for (const auto &[unit, position] : positions) {
    *unit->positions.reserveOne() = position;
    unit->positions.checkAndPush(&state);
  }
  rollback.committed = true;
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
    f5 = netutils::UNPACKS(pi_vals[1], PI);
    f6 = netutils::UNPACKS(pi_vals[2], PI);
    ok &= bs.Read(some_position_maybe);
    return ok;
  }
};


// AI guns have no unit object; their state is read all the same, into ev only.
// The vehicle state a decoded record changes outside ev/weapons is collected here and published
// by GMSync only after the whole record has been read. A read failure partway through the record
// then drops the update instead of leaving the position history or the turret state half applied.
struct VehiclePublish {
  struct TurretState {
    unit::Weapon *weapon;
    Point3 angles;
  };
  unit::Unit *position_unit = nullptr;
  SpaceTimeEuler position{};
  std::vector<TurretState> turret_states{};
};

bool ParseVehicleInfo(ParserState &state, BitStream &bs, TankRef *ref, mpi::GroundSyncEvent &ev,
                      mpi::GroundWeaponSync *weapons, VehiclePublish &publish) {
  ZoneScoped;
  RET_FAIL(bs.Read(ev.spawn_counter));
  bool b1;
  RET_FAIL(bs.Read(b1));
  ev.set(ev.HasMotion, b1);
  if (b1) {
    Point3 direction{};
    if (ref->val1) {
      float floats[4];
      RET_FAIL(bs.Read(floats)); // 4 float reads, then 1 later (usePositionHistoryForSlaves is true
      if (true) {
        bool extra;
        RET_FAIL(bs.Read(extra));
        if (extra) {
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
      // LOGI("{} {} {}", direction.x, direction.y, direction.z);
    }
    bool bool3, awake = false;
    RET_FAIL(bs.Read(awake));
    ev.set(ev.Awake, awake);
    RET_FAIL(bs.Read(bool3));
    Point3 unit_position{};
    if (bool3) {
      RET_FAIL(bs.Read(unit_position));
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
    if (ref->ref_1 && ref->ref_1->isGround()) {
      // Held back until the record is known good, see VehiclePublish.
      publish.position_unit = ref->ref_1;
      publish.position.euler = direction;
      publish.position.location = unit_position;
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
    ev.velocity = out_2;
    RET_FAIL(bs.Read(ev.angular_raw));
    bool has_drive;
    RET_FAIL(bs.Read(has_drive));
    if (has_drive) {
      // The track scale is measured: the same 0.1266 m/s per step on 21 vehicle types.
      RET_FAIL(bs.Read(ev.drive_raw));
      ev.set(ev.HasDrive, true);
    }
    RET_FAIL(bs.Read(ev.engine));
    // Only a ship sends this block. A ship then sends no flag_b6 and no alive bit; the rest of its state is
    // the same as a tank's (all 233,534 ship states of 3 naval battles read to the end).
    bool bool5;
    RET_FAIL(bs.Read(bool5));
    ev.set(ev.IsShip, bool5);
    if (bool5) {
      // A ship sends the drive bytes as zeros.
      ev.set(ev.HasDrive, false);
      uint8_t ship[6]; // s0..s2 water levels (probable), s3 = s4 throttle, s5 rudder
      uint8_t some_vals_2[2]; // never sent
      uint8_t w[4]; // w0..w2 unknown (127 without flooding), then the shaft count
      RET_FAIL(bs.Read(ship));
      bool has_extra;
      RET_FAIL(bs.Read(has_extra));
      if (has_extra) {
        RET_FAIL(bs.Read(some_vals_2));
      }
      bool flooding;
      RET_FAIL(bs.Read(flooding));
      ev.set(ev.ShipFlooding, flooding);
      RET_FAIL(bs.Read(w));
      RET_FAIL(w[3] <= ev.MaxShafts);
      std::copy_n(ship, 3, ev.ship_water);
      ev.ship_throttle_raw = ship[3];
      ev.ship_rudder_raw = ship[5];
      ev.shaft_count = w[3];
      for (uint8_t i = 0; i < ev.shaft_count; i++) {
        RET_FAIL(bs.Read(ev.shafts[i]));
      }
    } else {
      bool bool6, alive;
      RET_FAIL(bs.Read(bool6));
      RET_FAIL(bs.Read(alive));
      ev.set(ev.Alive, alive);
    }
    bool bool8;
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

    bool has_sub_vehicles;
    RET_FAIL(bs.Read(has_sub_vehicles));
    if (has_sub_vehicles) {
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
  bool b2, player_controlled;
  RET_FAIL(bs.Read(b2));
  RET_FAIL(bs.Read(player_controlled));
  ev.set(ev.PlayerControlled, player_controlled);
  float turret_horizontal = 5000;
  for (int i = 0; i < ref->turret_count; i++) {
    auto weap = ref->ref_1 ? ref->ref_1->getWeapon(i) : nullptr;
    auto &wev = weapons[i];
    bool bool1, bool2, bool3, bool4;
    uint8_t val1;
    RET_FAIL(bs.Read(bool1));
    wev.flags |= wev.HasBroken | (bool1 ? wev.IsBroken : 0);
    RET_FAIL(bs.Read(val1));
    RET_FAIL(bs.Read(bool2));
    if (bool2) {
      RET_FAIL(bs.Read(wev.heat_raw));
      wev.flags |= wev.HasHeat;
    }
    RET_FAIL(bs.Read(bool3));
    RET_FAIL(bs.Read(bool4));
    // Nothing of these carries ammo, which was measured rather than assumed: v1 is zero
    // in all 1.87 M samples of a battle. v2 is barrel heat, not the reload progress: it
    // gains about 50 per main-gun shot, does not reset when the reload ends, and stays at
    // 255 on a firing autocannon. A machine gun mount does not send it.
    // if (i==0)
    // LOGI("turret {}: bool1: {}; val1: {}; bool2: {}; val2: {}; bool3: {}; bool4: {}", i, bool1, val1, bool2, val2,
    //      bool3, bool4);
    if (b2) {
      bool has_turret;
      RET_FAIL(bs.Read(has_turret));
      if (has_turret) {
        uint16_t tv1, tv2;
        uint8_t tv18, tv28;
        float turret_x = 0.f, turret_y = 0.f, turret_z = 0.f;
        float rot11, rot22;
        RET_FAIL(bs.Read(tv1));
        RET_FAIL(bs.Read(tv2));
        turret_x = UNPACK_S16(tv1, PI);
        turret_y = UNPACK_S16(tv2, PI);
        wev.yaw_raw = int16_t(tv1);
        wev.pitch_raw = int16_t(tv2);
        wev.flags |= wev.HasTurret;
        // The same two angles in 1/256 of a turn; up to 20 deg apart while the turret turns fast.
        RET_FAIL(bs.Read(tv18));
        RET_FAIL(bs.Read(tv28));
        rot11 = UNPACK_S16(tv18, 180.0f);
        rot22 = UNPACK_S16(tv28, 180.0f);
        bool extra_stuff;
        RET_FAIL(bs.Read(extra_stuff));
        if (extra_stuff) {
          uint16_t extra_val;
          RET_FAIL(bs.Read(extra_val));
          turret_z = UNPACK_S16(extra_val, PI);
          //;LOGI("extra_val: {}", extra_val);
        }
        // A ship sends its preset weapons first, then commonWeapons: not the unit's weapon order.
        if (weap && weap->turret_desc && !ev.is_ship()) {
          publish.turret_states.push_back({weap, {turret_x, turret_y, turret_z}});
        }
      }
    }
  }
  if (ref->ref_1 && turret_horizontal <= 4000.f && b1) {
    auto &un_arr = ref->ref_1->positions;
    // if (!un_arr.empty()) {
    //   un_arr.back().turret_horizontal = turret_horizontal;
    // }
  }
  uint8_t sensorsCount{};
  RET_FAIL(bs.Read(sensorsCount));
  RET_FAIL(sensorsCount <= SENSORS_COUNT);
  thread_local std::vector<SensorsControlStates> states{};
  states.assign(sensorsCount, {});
  for (auto &sensor: states) {
    RET_FAIL(sensor.deserialize(bs));
  }
  uint8_t some_weird_val = 0;
  if (sensorsCount > 0) {
    RET_FAIL(bs.Read(some_weird_val));
  }
  if (ref->ref_1)
    for (uint8_t i = 0; i < sensorsCount; i++)
      if (state.keep.sensors)
        state.SensorEvents.push_back({state.curr_time_ms, ref->ref_1, i, some_weird_val, states[i]});
  uint8_t counterMeasuresCount;
  RET_FAIL(bs.Read(counterMeasuresCount));
  RET_FAIL(counterMeasuresCount <= COUNTER_MEASURES_COUNT);
  thread_local std::vector<CounterMeasuresControlState> counterMeasures{};
  counterMeasures.resize(counterMeasuresCount);
  for (auto &c: counterMeasures) {
    RET_FAIL(c.deserialize(bs));
  }
  uint8_t targetsNum = 0;
  RET_FAIL(bs.ReadBits(&targetsNum, 4));
  RET_FAIL(targetsNum <= 8);
  thread_local std::vector<TargetDesignationControlState> targets{};
  targets.assign(targetsNum, {});
  for (uint8_t i = 0; i < targetsNum; i++) {
    RET_FAIL(targets[i].deserialize(bs));
    if (ref->ref_1)
      if (state.keep.designations)
        state.DesignationEvents.push_back({state.curr_time_ms, ref->ref_1, i, targets[i]});
  }
  // The turret state is published by GMSync once the record is known good, see VehiclePublish.
  return true;
}


bool GMSync(ParserState &state, BitStream &bs) {
  ZoneScoped;
  uint16_t uid_lower;
  uint16_t uid_upper;
  float time_at;
  uint16_t packet_reliability_order;
  RET_FAIL(bs.Read(uid_lower));
  RET_FAIL(bs.Read(uid_upper));
  uint16_t uid_lower_base = uid_lower;
  RET_FAIL(bs.Read(time_at));
  RET_FAIL(bs.Read(packet_reliability_order));
  // A packet holds the records of several vehicles. Nothing of it is published until all of them are
  // read: a failed read drops the events, pools and ground syncs the earlier records appended.
  struct PacketRollback {
    ParserState &s;
    size_t sensors, designations, seekers, weapons, rounds, syncs;
    bool active = true;
    ~PacketRollback() {
      if (!active)
        return;
      s.SensorEvents.resize(sensors);
      s.DesignationEvents.resize(designations);
      s.SeekerEvents.resize(seekers);
      s.GroundWeapons.resize(weapons);
      s.GroundRounds.resize(rounds);
      s.GroundSyncEvents.resize(syncs);
    }
  } packet_rollback{state,
                    state.SensorEvents.size(),
                    state.DesignationEvents.size(),
                    state.SeekerEvents.size(),
                    state.GroundWeapons.size(),
                    state.GroundRounds.size(),
                    state.GroundSyncEvents.size()};
  std::vector<std::pair<unit::Unit *, VehiclePublish>> publishes;
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
      bool has_some_val;
      RET_FAIL(bs.Read(has_some_val));
      if (has_some_val) {
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
        mpi::GroundSyncEvent ev{state.curr_time_ms, uid_lower};
        ev.unit = ref.ref_1;
        ev.weapon_count = ref.turret_count;
        // With the list kept, the weapons go to the pool, and are dropped again when the event is not
        // kept (no state or a failed read); else to a scratch array.
        const bool keep = state.keep.ground_syncs;
        struct PoolRollback {
          ParserState &s;
          size_t w, r;
          bool active;
          ~PoolRollback() {
            if (active) {
              s.GroundWeapons.resize(w);
              s.GroundRounds.resize(r);
            }
          }
        } rollback{state, state.GroundWeapons.size(), state.GroundRounds.size(), keep};
        // State the record collects before it is known good; published after the last field is read.
        VehiclePublish publish{};
        thread_local std::vector<mpi::GroundWeaponSync> scratch{};
        mpi::GroundWeaponSync *weapons;
        if (keep) {
          ev.weapons_begin = uint32_t(state.GroundWeapons.size());
          state.GroundWeapons.resize(ev.weapons_begin + ref.turret_count);
          weapons = state.GroundWeapons.data() + ev.weapons_begin;
        } else {
          scratch.assign(ref.turret_count, {});
          weapons = scratch.data();
        }
        bool have_state = false;
        if (isCompressed) {
          auto &hist = state.NetDelta.getHistory(uid_lower);
          auto res = state.NetDelta.netDelta.readDelta(bs, &hist);
          if (res.ok) {
            have_state = ParseVehicleInfo(state, res.bs, &ref, ev, weapons, publish);
            if (have_state == false && ref.ref_1 != nullptr) {
              LOGE("Failed to parse info for ground unit {} of uid {}", ref.ref_1->unit_name, ref.ref_1->uid);
            }
          }
        } else {
          RET_FAIL(bs.GetNumberOfUnreadBits() >= ((8 - (bs.GetReadOffset() & 7)) & 7));
          bs.AlignReadToByteBoundary();
          have_state = ParseVehicleInfo(state, bs, &ref, ev, weapons, publish);
          if (!have_state)
            return false; // The remaining fields cannot be located after a short state.
        }
        bool has_aim;
        RET_FAIL(bs.Read(has_aim));
        if (has_aim) {
          // The point under the player's aim: minus the heading in degrees, the pitch in degrees, and
          // the distance twice. Sent exactly while the state says a player sits in the vehicle.
          float aim[4];
          RET_FAIL(bs.Read(aim));
          ev.aim[0] = -aim[0] * DEG_TO_RAD;
          ev.aim[1] = aim[1] * DEG_TO_RAD;
          ev.aim[2] = aim[2];
          ev.set(ev.HasAim, true);
        }
        bool bool8;
        RET_FAIL(bs.Read(bool8));
        if (bool8 && ref.turret_count > 0) { // maybe has_shot_gun???
          for (int i = 0; i < ref.turret_count; i++) {
            bool t1;
            RET_FAIL(bs.Read(t1));
            if (t1) {
              auto &w = weapons[i];
              uint8_t ammo_types;
              float last_shot, ready;
              uint8_t count;
              RET_FAIL(bs.Read(ammo_types));
              RET_FAIL(bs.Read(last_shot));
              RET_FAIL(bs.Read(ready));
              RET_FAIL(bs.Read(count));
              w.flags |= w.HasAmmo;
              w.ammo_types = ammo_types;
              w.last_shot_s = last_shot;
              w.ready_s = ready;
              w.round_count = count;
              w.rounds_begin = uint32_t(state.GroundRounds.size());
              for (uint8_t k = 0; k < count; k++) {
                uint16_t r;
                RET_FAIL(bs.Read(r));
                if (keep)
                  state.GroundRounds.push_back(r);
              }
            }
          }
        }
        bool has_sensor_info;
        bool has_sensor_info_2;
        RET_FAIL(bs.Read(has_sensor_info));
        if (has_sensor_info) {
          RET_FAIL(bs.Read(has_sensor_info_2));
          if (has_sensor_info_2) {
            uint32_t bits;
            RET_FAIL(bs.Read(bits));
            mpi::SeekerEvent event{state.curr_time_ms, mpi::SeekerGround};
            event.unit = ref.ref_1;
            RET_FAIL(ReadSeekerBits(state, bs, bits, event));
          }
        }
        RET_FAIL(bs.GetNumberOfUnreadBits() >= ((8 - (bs.GetReadOffset() & 7)) & 7));
        bs.AlignReadToByteBoundary();
        if (have_state)
          publishes.emplace_back(ref.ref_1, std::move(publish));
        if (keep && have_state) {
          state.GroundSyncEvents.push_back(ev);
          rollback.active = false;
        }
      }
    }
  }
  // Every record is read: publish the vehicle states they described.
  for (auto &[unit, publish]: publishes) {
    if (publish.position_unit) {
      *publish.position_unit->positions.reserveOne() = publish.position;
      publish.position_unit->positions.checkAndPush(&state);
    }
    if (unit) {
      for (auto &turret_state: publish.turret_states)
        turret_state.weapon->turret_desc->setData(turret_state.angles);
      unit->calculateTurretData();
      for (auto &weapon: unit->weapons)
        if (weapon.turret_desc)
          weapon.turret_desc->push_curr_state(state);
    }
  }
  packet_rollback.active = false;
  return true;
}

// A store leaving a pylon can be registered as any of these four, and which one it
// gets is not something the position packet says: it names the entity, not the kind.
// ParseWeapon throws the position away when the component it asked for is missing,
// so ask for the one the list is named after first and then for the rest. Payload
// and jettisoned were asked for by nobody at all, which cost such a store its whole
// trajectory.
static Rocket *getAnyStoreImpl(ParserState &state, ecs::EntityId eid) {
  if (auto *r = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("rocket_component")))
    return r;
  if (auto *b = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("bomb_component")))
    return b;
  if (auto *p = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("payload_component")))
    return p;
  if (auto *j = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("jettisoned_component")))
    return j;
  return state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("torpedo_component"));
}

static Rocket *getAnyStore(ParserState &state, ecs::EntityId eid, StoreType actual) {
  auto *ret = getAnyStoreImpl(state, eid);
  if (ret)
    LOGD3("entity '{}' has store of type {} but the position packet requested {}", eid, ret->type, actual);
  return ret;
}


Rocket *getRocket(ParserState &state, ecs::EntityId eid) {
  if (auto *r = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("rocket_component")))
    return r;
  return getAnyStore(state, eid, StoreType::Rocket);
}


Rocket *getBomb(ParserState &state, ecs::EntityId eid) {
  if (auto *b = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("bomb_component")))
    return b;
  return getAnyStore(state, eid, StoreType::Bomb);
}


Rocket *getTorpedo(ParserState &state, ecs::EntityId eid) {
  if (auto *t = state.g_entity_mgr.getNullable<Rocket>(eid, ECS_HASH("torpedo_component")))
    return t;
  return getAnyStore(state, eid, StoreType::Torpedo);
}

typedef Rocket *(*get_weapon_cb)(ParserState &state, ecs::EntityId eid);

bool ParseWeapon(ParserState &state, const BitStream &bs, get_weapon_cb cb) {
  bool bool1, bool2;
  RET_FAIL(bs.Read(bool1));
  RET_FAIL(bs.Read(bool2));
  ecs::EntityId eid;
  RET_FAIL(bs.Read(eid));
  Rocket *entity = cb(state, eid);
  // RET_FAIL(entity); // this can fucking happen apparently
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
  SpaceTimeEuler *back = nullptr;
  if (entity) {
    back = entity->positions.reserveOne();
    back->location = WeaponPos;
    entity->positions.checkAndPush(&state); // we don't compare, so this shouldn't invalidate our pointer
  }
  if (!bool2) {
    // Server time in ticks of 1/48 s, 0 to 42 ms ahead of the packet time.
    int tick;
    RET_FAIL(bs.ReadZigZag(tick));
    mpi::StoreSyncEvent sync{state.curr_time_ms, eid, (float) tick / 48.f};
    if (back) {
      RET_FAIL(bs.Read(back->euler));
    } else {
      Point3 b6;
      RET_FAIL(bs.Read(b6));
    }
    int16_t vel[3];
    RET_FAIL(bs.Read(vel));
    sync.velocity = Point3(vel[0], vel[1], vel[2]) * (2000.f / 32767.f);
    // Three more int16 near 0, x always 0: not the angular rate and not the lateral load.
    int16_t b8[3];
    RET_FAIL(bs.Read(b8));

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
    if (flags & 1)
      sync.flight_time = sf1;
    if (state.keep.store_syncs)
      state.StoreSyncEvents.push_back(sync);
    if (!bool1) {
      float vv;
      RET_FAIL(bs.Read(vv));
      bool vv2;
      RET_FAIL(bs.Read(vv2));
      uint16_t sz;
      RET_FAIL(bs.Read(sz));
      mpi::SeekerEvent ev{state.curr_time_ms, mpi::SeekerWeapon, eid};
      ev.lost_for = vv;
      if (flags & 1)
        ev.flight_time = sf1;
      ev.unknown_header_bit = vv2;
      RET_FAIL(ReadSeekerBits(state, bs, sz, ev));
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
  ZoneScoped;
  uint8_t rocket_count;
  RET_FAIL(bs.Read(rocket_count));
  for (int i = 0; i < rocket_count; i++) {
    ZoneScopedN("WeaponSync::ParseRocket");
    RET_FAIL(ParseWeapon(state, bs, getRocket));
  }

  uint8_t bomb_count;
  RET_FAIL(bs.Read(bomb_count));
  for (int i = 0; i < bomb_count; i++) {
    ZoneScopedN("WeaponSync::ParseBomb");
    RET_FAIL(ParseWeapon(state, bs, getBomb));
  }

  uint8_t torpedo_count;
  RET_FAIL(bs.Read(torpedo_count));
  for (int i = 0; i < torpedo_count; i++) {
    ZoneScopedN("WeaponSync::ParseTorpedo");
    RET_FAIL(ParseWeapon(state, bs, getTorpedo));
  }
  return true;
}
