#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "danet/BitStream.h"
#include "math/dag_Point3.h"
#include "ecs/entityId.h"

#define COUNTER_MEASURES_COUNT 2
#define SENSORS_COUNT          4
#define TARGETS_NUM            8


struct SensorsControlStates {
  // a bunch of this is probably a union actually
  bool v1 = 0;
  bool v2 = 0;
  bool first_bool = false; // maybe is turned on?
  bool has_state = false; ///< kind 1: the record carries the values below the type byte
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

  // The meaning below was checked on 2.59 server and client replays of jet battles.

  /// Sensor kind: 1 radar, IRST or optical tracker (the transceiver tells which), 2 laser.
  uint8_t kind() const { return sensor_type_maybe >> 4; }
  /// Index of the sensor in the vehicle's `sensors { sensor {...} }` list (aircraft
  /// flightmodels/*.blk, ground units/tankmodels/*.blk).
  uint8_t slot() const { return sensor_type_maybe & 0xf; }
  bool on() const { return first_bool; }
  /// Indexes into the sensor blk (gamedata/sensors/*.blk), each in the key order of its
  /// block: `transivers`, `scanPatterns`, `signals`. None when the sensor has none.
  std::optional<uint8_t> transceiver() const {
    return kind1() && field136_0x88 != 0xf ? std::optional<uint8_t>(field136_0x88) : std::nullopt;
  }
  std::optional<uint8_t> scan_pattern() const {
    return kind1() && field137_0x89 != 0x3f ? std::optional<uint8_t>(field137_0x89) : std::nullopt;
  }
  std::optional<uint8_t> signal() const {
    return kind1() && field138_0x8a != 0xf ? std::optional<uint8_t>(field138_0x8a) : std::nullopt;
  }
  /// Battle time in seconds of the last mode change.
  std::optional<float> mode_time() const { return kind1() ? std::optional<float>(some_data_1) : std::nullopt; }
  /// Scan centre in radians, relative to the vehicle: yaw about +Y, then pitch about +Z
  /// (after the vehicle's own yaw, pitch and roll). In search, the zone the player set; in
  /// track, toward the target, up to the scan pattern's azimuth and elevation limits.
  std::optional<float> scan_az() const { return kind1() ? std::optional<float>(some_data_4) : std::nullopt; }
  std::optional<float> scan_el() const { return kind1() ? std::optional<float>(some_data_5) : std::nullopt; }
  /// Unit uids of the targets the sensor detects at this sync (search, TWS track files, the
  /// tracked target). A contact names a unit as 0xFFFF0000 | uid; other values are left out.
  std::vector<uint16_t> contact_uids() const {
    std::vector<uint16_t> out;
    for (uint32_t c: field4_0x4)
      if ((c >> 16) == 0xFFFF)
        out.push_back(c & 0xFFFF);
    return out;
  }

private:
  /// A kind-1 record that is on and has its state carries the values above.
  bool kind1() const { return first_bool && kind() == 1 && has_state; }
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

  /// Unit uid of the designated target (the radar's track, or its TWS designation). Only a
  /// kind-6 record whose v15 is 0xFFFF0000 | uid names one; other v15 values are not uids.
  std::optional<uint16_t> target_uid() const {
    return v1 == 6 && (v15 >> 16) == 0xFFFF ? std::optional<uint16_t>(v15 & 0xFFFF) : std::nullopt;
  }
};

struct CounterMeasuresControlState {
  uint8_t v1;
  uint8_t v2;

  bool deserialize(BitStream &bs);
};

namespace unit {
  class Unit;
}

namespace mpi {
  /// One sensor of one unit at one sync. An aircraft sends its sensors with every flight
  /// update; a ground vehicle with every vehicle update that carries its state.
  struct SensorEvent {
    uint32_t time_ms = 0;
    unit::Unit *unit = nullptr;
    uint8_t index = 0; ///< position of the sensor in the unit's list for this sync
    /// Byte that follows the sensor list of the sync, the same for every sensor in it.
    uint8_t list_tail = 0;
    SensorsControlStates state{};
  };

  /// One target designation of one unit at one sync.
  struct DesignationEvent {
    uint32_t time_ms = 0;
    unit::Unit *unit = nullptr;
    uint8_t index = 0;
    TargetDesignationControlState state{};
  };

  /// One engine of an aircraft at one flight-model sync.
  struct EngineSync {
    uint8_t state = 0; ///< 7 while the engine runs, 8 once it has stopped; 0 and 6 occur for short times
    /// Afterburner throttle of a jet, 1.0 to 1.1, sent only while above 1.0. A prop never
    /// sends it, also at WEP.
    std::optional<float> afterburner{};
    /// Sent only while below 1.0. It falls in steps when the engine is hit and is 0 once
    /// the engine has stopped: the engine's health.
    std::optional<float> health{};
    /// 0 to 255, always 0 on a jet. On a prop they open as the coolant gets hot: the
    /// radiator flaps.
    uint8_t radiators[2]{};
  };

  /// The pilot's controls and the engines of one aircraft at one flight-model sync.
  struct ControlEvent {
    uint32_t time_ms = 0;
    unit::Unit *unit = nullptr;
    bool on_ground = false;
    /// The seven control bytes as sent; bytes 3, 4 and 6 have only been seen as 0.
    uint8_t controls[7]{};
    std::vector<EngineSync> engines{};

    /// Stick and pedals, -1 to 1 in steps of 1/7, sign as sent. Pitch is positive for a pull.
    float pitch() const { return ((controls[0] & 15) - 8) / 7.f; }
    float roll() const { return ((controls[1] >> 4) - 8) / 7.f; }
    float rudder() const { return ((controls[1] & 15) - 8) / 7.f; }
    /// 0 to 1 in steps of 1/7.
    float flaps() const { return ((controls[0] >> 4) & 7) / 7.f; }
    /// 0 to 1 in steps of 1/15.
    float airbrake() const { return (controls[2] & 15) / 15.f; }
    float wheel_brake() const { return (controls[2] >> 4) / 15.f; }
    /// The throttle lever, 0 to 1 in steps of 1/15. Above 100% it stays at 1: see
    /// EngineSync::afterburner.
    float throttle() const { return (controls[5] & 15) / 15.f; }
  };

  enum SeekerSource : uint8_t {
    SeekerWeapon = 0, ///< a guided store in flight, from WeaponSync
    SeekerAircraft = 1, ///< the seeker of a store still on an aircraft, from FMSync
    SeekerGround = 2, ///< the seeker of a ground vehicle's missile, from GMSync
  };

  /// A seeker block, kept as the raw bits; DecodeSeeker reads the known fields.
  struct SeekerEvent {
    uint32_t time_ms = 0;
    SeekerSource source = SeekerWeapon;
    ecs::EntityId eid{}; ///< the store, for SeekerWeapon
    unit::Unit *unit = nullptr; ///< the carrier, for SeekerAircraft and SeekerGround
    /// SeekerWeapon only: seconds since the seeker lost its target, 0 while it holds a lock
    /// and before its first lock. A lock on a flare counts as a lock.
    float lost_for = 0;
    /// SeekerWeapon only: seconds since launch, in steps of 1/48 s.
    std::optional<float> flight_time{};
    bool head_b = false; ///< SeekerWeapon only: the bit before the block
    uint32_t bits = 0;
    /// The bits as BitStream::ReadBits gives them: whole bytes first, the last
    /// partial byte right-aligned.
    std::vector<uint8_t> data{};
  };

  /// The decoded fields of a seeker block. The block length tells the seeker. Of a store in
  /// flight: 607 bits radar, 639 bits radar at the change from search to track, 283 bits IR.
  /// Of a store still on an aircraft: 518 bits (IR, or no target), 599 bits (radar, no
  /// target) and 895 bits (radar target); only some of their fields are known.
  struct SeekerState {
    bool decoded = false;
    /// True in track, false in search; empty for an IR seeker, whose lock bit is not known.
    std::optional<bool> tracking{};
    /// Unit line of sight from the missile, world axes.
    Point3 los{};
    /// Radar only: the seeker's range in metres. It reads 0 to 15% above the true distance.
    std::optional<float> range{};
    /// Radar only: the seeker's estimate of the target position, world axes. In the 895-bit
    /// block of an aircraft, the target the missile gets at launch.
    std::optional<Point3> target_pos{};
    /// 518-bit aircraft block: 0 no lock (also after each launch), 2 a new lock, 3 and 4 lock held.
    std::optional<uint8_t> ir_state{};
    /// 518-bit aircraft block: battle time in seconds of the last seeker lock; empty before the first.
    std::optional<float> lock_time{};
  };

  SeekerState DecodeSeeker(const SeekerEvent &ev);
} // namespace mpi
