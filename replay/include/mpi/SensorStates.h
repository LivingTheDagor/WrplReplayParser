#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include "daNet/bitStream.h"
#include "daNet/dag_netUtils.h"
#include "math/dag_mathBase.h"
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
  int unknown_field_149 = 0; ///< signed 2-bit value, present when has_unknown_fields_149_150
  int unknown_field_150 = 0; ///< signed 2-bit value, present when has_unknown_fields_149_150
  uint8_t contact_count = 0; ///< 6-bit count of contacts
  uint8_t contacts_tail = 0; ///< 6-bit value after the contacts, meaning unknown
  std::vector<uint32_t> contacts{}; ///< see contact_uids()
  uint8_t sensor_type_maybe = 0;
  uint8_t field136_0x88 = 0;
  uint8_t field137_0x89 = 0;
  uint8_t field138_0x8a = 0;
  float unknown_field_1{}; ///< kind 1: mode_time(); kind 2: normalized int16 times pi; kind 4: float32
  float unknown_field_2{}; ///< unknown field 2: kind 1 normalized int16; kinds 2/4 float32
  float unknown_field_3{}; ///< unknown field 3: float32, sent for kinds 2/4 when their data is present
  float unknown_field_4{}; ///< kind 1: scan_az(); kind 2: float32
  float unknown_field_5{}; ///< kind 1: scan_el(); kind 2: float32
  /// Field 6: four raw bytes for kind 2; kind 1 byte 0 is raw and byte 1 holds a decoded 1-bit value.
  char unknown_field_6_data[4]{};
  int32_t unknown_field_7{}; ///< kind 2 only, int32
  float unknown_field_147{}; ///< unknown field 147: float32, sent for kind 2 when on
  bool has_unknown_field_6_byte0 = false;
  bool has_unknown_fields_149_150 = false; ///< also indicates field 6 byte 1 is present
  bool has_kind4_state = false;

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
  std::optional<float> mode_time() const { return kind1() ? std::optional<float>(unknown_field_1) : std::nullopt; }
  /// Scan centre in radians, relative to the vehicle: yaw about +Y, then pitch about +Z
  /// (after the vehicle's own yaw, pitch and roll). In search, the zone the player set; in
  /// track, toward the target, up to the scan pattern's azimuth and elevation limits.
  std::optional<float> scan_az() const { return kind1() ? std::optional<float>(unknown_field_4) : std::nullopt; }
  std::optional<float> scan_el() const { return kind1() ? std::optional<float>(unknown_field_5) : std::nullopt; }
  /// Unit uids of the targets the sensor detects at this sync (search, TWS track files, the
  /// tracked target). A contact names a unit as 0xFFFF0000 | uid; other values are left out.
  std::vector<uint16_t> contact_uids() const {
    std::vector<uint16_t> out;
    for (uint32_t c: contacts)
      if ((c >> 16) == 0xFFFF)
        out.push_back(c & 0xFFFF);
    return out;
  }

private:
  /// A kind-1 record that is on and has its state carries the values above.
  bool kind1() const { return first_bool && kind() == 1 && has_state; }
};

struct TargetDesignationControlState {
  // Numbers are stable identifiers from the original decoder, not semantic names.
  // Point fields have three float32 components; their units and frames are unknown.
  uint8_t unknown_field_1{}; ///< 8-bit, always present
  uint8_t unknown_field_2{}; ///< 8-bit, always present
  bool has_unknown_field_4{}; ///< 1-bit presence flag, formerly field 3
  Point3 unknown_field_4{}; ///< three float32 components, present when has_unknown_field_4
  bool uses_compressed_vector{}; ///< selects fields 6/7 and the encoding of field 9
  float unknown_field_5{}; ///< float32, always present
  Point3 unknown_field_6{}; ///< three float32 components, only when uses_compressed_vector
  Point3 unknown_field_7{}; ///< three float32 components, only when not uses_compressed_vector
  bool unknown_field_8{}; ///< 1-bit, always present
  /// Three decoded float32 components, always present. Compressed: three signed 16-bit values
  /// scaled to 4000; otherwise three float32 values. Units and reference frame are unknown.
  Point3 unknown_field_9{};
  uint8_t unknown_field_10{}; ///< 1-bit value, always present
  float unknown_field_11{}; ///< normalized unsigned 8-bit value, only when has_unknown_field_11
  bool unknown_field_12{}; ///< 1-bit bool, always present
  bool unknown_field_13{}; ///< 1-bit, always present
  uint8_t unknown_field_14{}; ///< 8-bit, only when has_unknown_field_14
  uint32_t unknown_field_15{}; ///< 32-bit, only when has_unknown_field_15
  bool has_unknown_field_11{};
  bool has_unknown_field_14{};
  bool has_unknown_field_15{};

  bool deserialize(BitStream &bs);

  /// Unit uid of the designated target (the radar's track, or its TWS designation). Only a
  /// kind-6 record whose field 15 is 0xFFFF0000 | uid names one; other values are not uids.
  std::optional<uint16_t> target_uid() const {
    return unknown_field_1 == 6 && has_unknown_field_15 && (unknown_field_15 >> 16) == 0xFFFF
             ? std::optional<uint16_t>(unknown_field_15 & 0xFFFF) : std::nullopt;
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

  /// One engine of an aircraft at one flight-model sync. The engine count equals the EngineN blocks of
  /// the flight model, rocket boosters included.
  struct EngineSync {
    /// 7 runs; 8 destroyed (health 0); a start on the ground goes 0, 1, 2, 3, 4, 5, 7; 6 is probably a
    /// shut-down; a rocket booster reads 0 while not in use. At a kill it goes to 8 on jets, mostly 0 on props.
    uint8_t state = 0;
    /// Engine throttle with afterburner, 1.0 to 1.1, sent only while above 1.0 and only by engines with
    /// Afterburner.Type 3 and AfterburnerBoost > 1.0. Never by props (also at WEP), jets without
    /// afterburner, water injection or boosters. The sync of a dead aircraft never has it.
    std::optional<float> afterburner{};
    /// Engine health, sent only below 1.0; it starts within 5 s of a hit and is 0 at state 8.
    std::optional<float> health{};
    /// Radiator positions 0 to 255, 0 on a jet. Equal with a common radiator; water and oil differ
    /// otherwise (which is which is not known). 0 for the other aircraft of a client replay.
    uint8_t radiators[2]{};
  };

  /// The pilot's controls and the engines of one aircraft at one flight-model sync.
  struct ControlEvent {
    uint32_t time_ms = 0;
    unit::Unit *unit = nullptr;
    bool on_ground = false;
    /// The seven control bytes as sent. Byte 6 is always 0. Probable: byte 3 is 15 with the bomb bay open,
    /// the high nibble of byte 5 is the wing sweep.
    uint8_t controls[7]{};
    std::vector<EngineSync> engines{};
    /// Gun triggers, sent only while a trigger is held: the round counters of the guns on
    /// that trigger fall in the same sync. On a P-63 the four .50s are on the machine-gun
    /// trigger and the 37 mm on the cannon trigger; a jet's gun is on the cannon trigger.
    bool fire_mg = false;
    bool fire_cannon = false;
    /// Trigger of the additional guns (flight-model trigger "additional gun": gun pods, guns below the wings).
    bool fire_additional = false;

    /// Stick and pedals, -1 to 1 in steps of 1/7, sign as sent. Pitch is positive for a pull.
    float pitch() const { return ((controls[0] & 15) - 8) / 7.f; }
    float roll() const { return ((controls[1] >> 4) - 8) / 7.f; }
    float rudder() const { return ((controls[1] & 15) - 8) / 7.f; }
    /// 0 to 1 in steps of 1/15.
    float flaps() const { return (controls[0] >> 4) / 15.f; }
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
    bool unknown_header_bit = false; ///< unknown 1-bit field before the block, SeekerWeapon only
    uint32_t bits = 0;
    /// The bits as BitStream::ReadBits gives them: whole bytes first, the last
    /// partial byte right-aligned.
    std::vector<uint8_t> data{};
  };

  /// One store (missile or rocket) at one 0xF11A store sync, beside its position in the store's history.
  /// 0xF11A carries no gun shells.
  struct StoreSyncEvent {
    uint32_t time_ms = 0;
    ecs::EntityId eid{};
    /// Server time in steps of 1/48 s; 0 to 42 ms after the packet time.
    float server_time = 0;
    /// World velocity, m/s (signed 16-bit component scaled by 2000 / 32767).
    Point3 velocity{};
    /// Seconds since launch, in steps of 1/48 s.
    std::optional<float> flight_time{};
  };

  /// One weapon of a ground vehicle at one sync, kept as sent; the accessors give the values. A part
  /// comes from the delta-coded state, the rest from the ammunition list after it; either can be missing.
  /// The records live in ParserState::GroundWeapons, their rounds in ParserState::GroundRounds.
  struct GroundWeaponSync {
    enum : uint8_t { HasBroken = 1, IsBroken = 2, HasHeat = 4, HasTurret = 8, HasAmmo = 16 };
    uint8_t flags = 0;
    uint8_t heat_raw = 0;
    uint8_t ammo_types = 0; ///< loaded type in the low nibble, selected type in the high one
    uint8_t round_count = 0;
    int16_t yaw_raw = 0;
    int16_t pitch_raw = 0;
    float last_shot_s = 0;
    float ready_s = 0;
    uint32_t rounds_begin = 0; ///< index into ParserState::GroundRounds

    /// Probable: the weapon is disabled. It rises within 1 s of a hit and falls without one (repair).
    std::optional<bool> broken() const { return has(HasBroken) ? std::optional<bool>(has(IsBroken)) : std::nullopt; }
    /// Probable: barrel heat, 0 to 1. A main gun gains about 0.2 per shot and loses about 0.016 per
    /// second; it reaches 1 on autocannons. Not sent for machine guns. It is not the reload progress.
    std::optional<float> heat() const {
      return has(HasHeat) ? std::optional<float>(netutils::UNPACK(heat_raw, 1.0)) : std::nullopt;
    }
    /// Turret yaw and gun elevation relative to the hull, radians. The yaw takes the sign of the hull
    /// heading: the F0CC gun yaw minus the heading equals it (median 0.31 deg).
    std::optional<float> turret_yaw() const { return angle(yaw_raw); }
    std::optional<float> turret_pitch() const { return angle(pitch_raw); }
    /// Ammunition type in the gun and the type that comes next, indexes into rounds.
    std::optional<uint8_t> loaded_type() const { return ammo<uint8_t>(ammo_types & 15); }
    std::optional<uint8_t> selected_type() const { return ammo<uint8_t>(ammo_types >> 4); }
    /// Battle time in seconds of the weapon's last shot when the ammo record is present.
    std::optional<float> last_shot() const { return ammo<float>(last_shot_s); }
    /// Raw ready field in seconds when the ammo record is present. Zero is possible even
    /// after a shot; its meaning and when it represents a usable timestamp are unknown.
    std::optional<float> ready() const { return ammo<float>(ready_s); }

  private:
    bool has(uint8_t f) const { return flags & f; }
    /// As UNPACK_S16(raw, PI), which gives -PI for the invalid value.
    std::optional<float> angle(int16_t raw) const {
      if (!has(HasTurret))
        return std::nullopt;
      return raw == INT16_MIN ? -PI : netutils::UNPACKS(raw, PI);
    }
    template <typename T>
    std::optional<T> ammo(T v) const {
      return has(HasAmmo) ? std::optional<T>(v) : std::nullopt;
    }
  };

  /// One ground vehicle (or ship, or AI gun) at one 0xF074 sync, kept as sent; the accessors give the
  /// values. The motion values are there only when has_motion is set. ParserState keeps an event only
  /// when it differs from the vehicle's previous one: the state at a time is the vehicle's last event
  /// at or before it.
  struct GroundSyncEvent {
    enum : uint8_t {
      HasMotion = 1, IsShip = 2, Awake = 4, Alive = 8, PlayerControlled = 16, HasDrive = 32, HasAim = 64,
      ShipFlooding = 128
    };
    static constexpr int MaxShafts = 9;
    uint32_t time_ms = 0;
    uint16_t uid = 0;
    /// Two 4-bit spawn counters of the player, constant for a vehicle's life; both rise at each
    /// new spawn. Which nibble counts what is not known.
    uint8_t spawn_counter = 0;
    uint8_t flags = 0;
    unit::Unit *unit = nullptr; ///< null for an AI gun
    Point3 velocity{}; ///< world axes, m/s
    int8_t angular_raw[3]{};
    int8_t drive_raw[4]{};
    /// Probable: engine speed, 0 to 255; 0 when the engine stops, also at the kill.
    uint8_t engine = 0;
    float aim[3]{}; ///< aim_yaw, aim_pitch, aim_distance as sent in the aim block
    /// Ship block. Probable: three water levels that rise with flooding; s0 also rises at the kill while
    /// the ship sinks.
    uint8_t ship_water[3]{};
    uint8_t ship_throttle_raw = 0;
    uint8_t ship_rudder_raw = 0;
    uint8_t shaft_count = 0;
    /// One per transmission_NN_dm part (propeller shaft), 255 intact, falls with hits.
    uint8_t shafts[MaxShafts]{};
    uint8_t weapon_count = 0;
    uint32_t weapons_begin = 0; ///< index into ParserState::GroundWeapons

    bool has_motion() const { return has(HasMotion); }
    /// The ship block was sent (some AI ships send the tank layout).
    bool is_ship() const { return has(IsShip); }
    /// Ship block. Throttle 127 stop, 254 full ahead, 0 full reverse; rudder 127 centre.
    std::optional<uint8_t> ship_throttle() const { return ship(ship_throttle_raw); }
    std::optional<uint8_t> ship_rudder() const { return ship(ship_rudder_raw); }
    /// Probable: flooding.
    bool ship_flooding() const { return has(ShipFlooding); }
    std::vector<uint8_t> ship_shafts() const { return {shafts, shafts + shaft_count}; }
    /// Rad/s about the body axes, in the order of the unit's euler: roll, yaw, pitch.
    Point3 angular_velocity() const {
      return {netutils::UNPACKS(angular_raw[0], PI), netutils::UNPACKS(angular_raw[1], PI),
              netutils::UNPACKS(angular_raw[2], PI)};
    }
    /// Probable: the physics body is not at rest. Never 0 while the vehicle moves.
    bool awake() const { return has(Awake); }
    /// Track (or wheel side) speeds in m/s, by the measured scale of 0.1266 m/s per step.
    std::optional<float> track_left() const { return drive(drive_raw[0] * 0.1266f); }
    std::optional<float> track_right() const { return drive(drive_raw[1] * 0.1266f); }
    /// Probable: steering input, -1 to 1.
    std::optional<float> steer() const { return drive(drive_raw[2] / 127.f); }
    /// Drive input: 1 forward, 0, -1 reverse.
    std::optional<float> throttle() const { return drive(drive_raw[3] / 127.f); }
    /// Probable: the vehicle is alive; falls within 100 ms of the kill. A ship does not send it.
    bool alive() const { return has(Alive); }
    /// A player sits in the vehicle. Falls about 10 s after the kill; 0 on AI guns.
    bool player_controlled() const { return has(PlayerControlled); }
    /// The point under the player's aim, while player_controlled: heading in radians (the sign of
    /// the hull heading), and probable: pitch in radians and the distance in metres.
    std::optional<float> aim_yaw() const { return aimed(aim[0]); }
    std::optional<float> aim_pitch() const { return aimed(aim[1]); }
    std::optional<float> aim_distance() const { return aimed(aim[2]); }

    void set(uint8_t f, bool on) { flags = on ? flags | f : flags & ~f; }

  private:
    bool has(uint8_t f) const { return flags & f; }
    std::optional<float> drive(float v) const { return has(HasDrive) ? std::optional<float>(v) : std::nullopt; }
    std::optional<float> aimed(float v) const { return has(HasAim) ? std::optional<float>(v) : std::nullopt; }
    std::optional<uint8_t> ship(uint8_t v) const { return is_ship() ? std::optional<uint8_t>(v) : std::nullopt; }
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
    /// 518-bit aircraft block: 0 no lock (also after each launch), 2 seeker on at the boresight, 4 the seeker
    /// axis on an aircraft (median 0.32 deg), 3 probable: a target, but the axis not on an aircraft. 6 also
    /// occurs, with another layout.
    std::optional<uint8_t> ir_state{};
    /// 518-bit aircraft block: battle time in seconds of the last change out of state 0 or 2; a change between
    /// 3 and 4 keeps it. Empty before the first.
    std::optional<float> lock_time{};
    /// 518-bit aircraft block: the seeker axis, a unit vector in world axes; in states 0 and 2 the aircraft's
    /// forward axis about 2 deg down.
    std::optional<Point3> seeker_dir{};
    /// 518-bit aircraft block: gimbal angles in radians relative to the aircraft, azimuth positive to the left
    /// and elevation positive up; 0 while caged, they stop at the missile's angleMax.
    std::optional<float> gimbal_az{};
    std::optional<float> gimbal_el{};
    /// 518-bit aircraft block: half the seeker fov in radians (0.0314 for an AIM-9M).
    std::optional<float> fov_half{};
  };

  SeekerState DecodeSeeker(const SeekerEvent &ev);
} // namespace mpi
