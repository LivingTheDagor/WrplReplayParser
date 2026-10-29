#include "modules/mpi/battle_messages.h"
#include "Unit.h"
#include "modules/ecs/EntityId.h"
#include "mpi/GeneralObject.h"
#include "mpi/LocalClientObject.h"
#include "mpi/SensorStates.h"
#include "modules/bind_readonly_vector.h"
PyBattleMessages py_battle_messages;
void PyBattleMessages::include(py::module_ &m) {
  DO_INCLUDE()
  py_entity_id.include(m);
  auto mpi = m.def_submodule("mpi");
  py::class_<mpi::IBattleMessage, std::unique_ptr<mpi::IBattleMessage, py::nodelete>>(mpi, "IBattleMessage")
      .def_readonly("time_ms", &mpi::IBattleMessage::time_ms);

  py::class_<mpi::KillMessage, mpi::IBattleMessage, std::unique_ptr<mpi::KillMessage, py::nodelete>>(mpi, "KillMessage")
      .def_readonly("offender_vehicle", &mpi::KillMessage::offender_vehicle)
      .def_readonly("used_weapon", &mpi::KillMessage::used_weapon)
      .def_readonly("destroyed_weapon", &mpi::KillMessage::destroyed_weapon)
      .def_readonly("offended_unit", &mpi::KillMessage::offended_unit)
      .def_readonly("offended_unit_position", &mpi::KillMessage::offended_unit_position)
      .def_readonly("offender_unit", &mpi::KillMessage::offender_unit)
      .def_readonly("offender_unit_position", &mpi::KillMessage::offender_unit_position)
      .def_readonly("offender_pid", &mpi::KillMessage::offender_pid)
      .def_readonly("VictimPid", &mpi::KillMessage::VictimPid)
      .def_readonly("unitType", &mpi::KillMessage::unitType)
      .def_readonly("is_burav_kill", &mpi::KillMessage::maybe_is_burav_kill)
      .def_readonly("death_type", &mpi::KillMessage::DeathType)
      .def_property_readonly(
        "death_reason",
        [](const mpi::KillMessage &m) { return std::string(mpi::deathReasonKey(m.DeathType)); },
        "How the vehicle died, keyed the way the game keys it: crewDeath, ammoFire, machOverspeed. "
        "death_type is an index into the game's own list of reasons, and this resolves it. Empty when "
        "the message carries no reason: a death_type of 0 is read as an unfilled field, which makes "
        "the reason it would otherwise name, death/byShip, unreachable.")
      .def_property_readonly("weapon_type",
                             [](const mpi::KillMessage &m) { return uint8_t(m.some_enum); },
                             "Munition class of the killing weapon: 1 bullet or shell, 2 bomb, 3 rocket, "
                             "4 torpedo, 0 when the kill names no weapon. Checked against used_weapon on "
                             "every kill of three replays.")
      .def_readonly("weapon_flags", &mpi::KillMessage::some_weap_flags,
                    "Bit flags of the killing weapon: bit 0 artillery, bit 1 depth bomb, bit 2 mine, "
                    "bit 5 unknown. Only bit 5 (value 32) was seen on the test replays, on gun kills "
                    "of ground vehicles; bit 0 is the flag an artillery strike would be read by.");

  py::class_<mpi::SevereDamageMessage, mpi::IBattleMessage, std::unique_ptr<mpi::SevereDamageMessage, py::nodelete>>(
      mpi, "SevereDamageMessage")
      .def_readonly("offended_unit", &mpi::SevereDamageMessage::offended_unit)
      .def_readonly("player_pid", &mpi::SevereDamageMessage::player_pid)
      .def_readonly("vehicle", &mpi::SevereDamageMessage::vehicle)
      .def_readonly("offender_unit", &mpi::SevereDamageMessage::offender_unit)
      .def_readonly("unitType", &mpi::SevereDamageMessage::unitType);

  py::class_<mpi::CriticalDamageMessage, mpi::IBattleMessage,
             std::unique_ptr<mpi::CriticalDamageMessage, py::nodelete>>(mpi, "CriticalDamageMessage")
      .def_readonly("offended_unit", &mpi::CriticalDamageMessage::offended_unit)
      .def_readonly("player_pid", &mpi::CriticalDamageMessage::player_pid)
      .def_readonly("vehicle", &mpi::CriticalDamageMessage::vehicle)
      .def_readonly("offender_unit", &mpi::CriticalDamageMessage::offender_unit)
      .def_readonly("is_fire", &mpi::CriticalDamageMessage::is_fire)
      .def_readonly("unitType", &mpi::CriticalDamageMessage::unitType);

  // Shot and hit packets. Records are kept as they arrived: no joining between the
  // streams, that is up to the caller. Only offender_unit is resolved here, because
  // a uid maps to a unit only at the moment the packet is read.
  py::class_<mpi::ProjectileKey>(mpi, "ProjectileKey")
      .def_readonly("offender_oid", &mpi::ProjectileKey::offender_oid)
      .def_readonly("projectile_id", &mpi::ProjectileKey::projectile_id)
      .def_readonly("generation", &mpi::ProjectileKey::generation)
      .def_property_readonly("offender_uid", &mpi::ProjectileKey::offender_uid)
      .def_property_readonly("offender_type", &mpi::ProjectileKey::offender_type)
      .def_property_readonly("projectile_uid", &mpi::ProjectileKey::projectile_uid,
                             "Same value 0xF15F carries as projectileUid")
      .def_property_readonly("valid", &mpi::ProjectileKey::valid)
      .def("__eq__", [](const mpi::ProjectileKey &a, const mpi::ProjectileKey &b) { return a == b; })
      .def("__hash__",
           [](const mpi::ProjectileKey &a) {
             return (uint64_t(a.offender_oid) << 24) | a.projectile_uid();
           })
      .def("__repr__", [](const mpi::ProjectileKey &a) {
        return "ProjectileKey(offender_oid=" + std::to_string(a.offender_oid) +
               ", projectile_id=" + std::to_string(a.projectile_id) +
               ", generation=" + std::to_string(a.generation) + ")";
      });

  py::class_<mpi::HitEffect>(mpi, "HitEffect")
      .def_readonly("time_ms", &mpi::HitEffect::time_ms)
      .def_readonly("projectile", &mpi::HitEffect::projectile)
      .def_readonly("offended_unit", &mpi::HitEffect::offended_unit)
      .def_readonly("offender_unit", &mpi::HitEffect::offender_unit)
      .def_readonly("local_pos", &mpi::HitEffect::local_pos)
      .def_readonly("local_dir", &mpi::HitEffect::local_dir)
      .def_readonly("complete", &mpi::HitEffect::complete);

  py::class_<mpi::HitAnalysis>(mpi, "HitAnalysis")
      .def_readonly("time_ms", &mpi::HitAnalysis::time_ms)
      .def_readonly("offender_oid", &mpi::HitAnalysis::offender_oid)
      .def_readonly("offended_unit", &mpi::HitAnalysis::offended_unit)
      .def_readonly("offender_unit", &mpi::HitAnalysis::offender_unit)
      .def_readonly("version", &mpi::HitAnalysis::version)
      .def_readonly("time_s", &mpi::HitAnalysis::time_s)
      .def_readonly("projectile_type", &mpi::HitAnalysis::projectile_type)
      .def_readonly("projectile_uid", &mpi::HitAnalysis::projectile_uid)
      .def_readonly("pos", &mpi::HitAnalysis::pos)
      .def_readonly("dir", &mpi::HitAnalysis::dir)
      .def_readonly("local_pos", &mpi::HitAnalysis::local_pos)
      .def_readonly("local_dir", &mpi::HitAnalysis::local_dir)
      .def_readonly("speed", &mpi::HitAnalysis::speed)
      .def_readonly("travel_distance", &mpi::HitAnalysis::travel_distance)
      .def_readonly("seed", &mpi::HitAnalysis::seed);

  py::class_<mpi::HitDamage>(mpi, "HitDamage")
      .def_readonly("time_ms", &mpi::HitDamage::time_ms)
      .def_readonly("crit", &mpi::HitDamage::crit,
                    "Damage model crit, not the critical hit line of the kill feed")
      .def_readonly("projectile", &mpi::HitDamage::projectile)
      .def_readonly("offended_unit", &mpi::HitDamage::offended_unit)
      .def_readonly("offender_unit", &mpi::HitDamage::offender_unit)
      .def_readonly("damage_class", &mpi::HitDamage::damage_class)
      .def_readonly("amount", &mpi::HitDamage::amount);

  py::class_<mpi::HitDirection>(mpi, "HitDirection")
      .def_readonly("time_ms", &mpi::HitDirection::time_ms)
      .def_readonly("offended_unit", &mpi::HitDirection::offended_unit)
      .def_readonly("world_dir", &mpi::HitDirection::world_dir);

  py::class_<mpi::HitExplosion>(mpi, "HitExplosion")
      .def_readonly("time_ms", &mpi::HitExplosion::time_ms)
      .def_readonly("projectile", &mpi::HitExplosion::projectile)
      .def_readonly("offended_unit", &mpi::HitExplosion::offended_unit)
      .def_readonly("offender_unit", &mpi::HitExplosion::offender_unit);

  py::class_<mpi::HitOutcome>(mpi, "HitOutcome")
      .def_readonly("time_ms", &mpi::HitOutcome::time_ms)
      .def_readonly("offender_oid", &mpi::HitOutcome::offender_oid)
      .def_readonly("projectile_uid", &mpi::HitOutcome::projectile_uid)
      .def_readonly("offended_unit", &mpi::HitOutcome::offended_unit)
      .def_readonly("offender_unit", &mpi::HitOutcome::offender_unit)
      .def_readonly("kinetic_parts", &mpi::HitOutcome::kinetic_parts)
      .def_readonly("cumulative_parts", &mpi::HitOutcome::cumulative_parts)
      .def_readonly("ricochet_parts", &mpi::HitOutcome::ricochet_parts)
      .def_readonly("fire_parts", &mpi::HitOutcome::fire_parts)
      .def_readonly("part_count", &mpi::HitOutcome::part_count)
      .def_readonly("changed_parts", &mpi::HitOutcome::changed_parts)
      .def_readonly("complete", &mpi::HitOutcome::complete);

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitOutcome>>(m, "HitOutcomeList");

  py::class_<mpi::AmmoEvent>(mpi, "AmmoEvent")
      .def_readonly("time_ms", &mpi::AmmoEvent::time_ms)
      .def_readonly("unit", &mpi::AmmoEvent::unit)
      .def_readonly("barrel", &mpi::AmmoEvent::barrel)
      .def_readonly("slot", &mpi::AmmoEvent::slot,
                    "Which load of this gun the count is for: an index over the Unit.weapons lines whose "
                    "bullet names a set of this gun, not over the whole loadout. Always 0 on aircraft, "
                    "where WeaponData.launcher names the gun instead.")
      .def_readonly("rounds_left", &mpi::AmmoEvent::rounds_left);

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::AmmoEvent>>(m, "AmmoEventList");

  py::enum_<mpi::ShotKind>(mpi, "ShotKind")
      .value("Single", mpi::ShotSingle)
      .value("FireStart", mpi::ShotFireStart)
      .value("FireStop", mpi::ShotFireStop);

  py::class_<mpi::CockpitEvent>(mpi, "CockpitEvent")
      .def_readonly("time_ms", &mpi::CockpitEvent::time_ms)
      .def_property_readonly("params", [](const mpi::CockpitEvent &e) {
        py::dict d;
        for (auto &[id, v]: e.params)
          d[py::int_(id)] = v;
        return d;
      }, "{id: value}; see CockpitEvent in GeneralObject.h for the ids that are known");

  py::class_<mpi::RpmEvent>(mpi, "RpmEvent")
      .def_readonly("time_ms", &mpi::RpmEvent::time_ms)
      .def_readonly("unit", &mpi::RpmEvent::unit)
      .def_readonly("rpm", &mpi::RpmEvent::rpm, "One per engine; a stopped jet engine reads 2000");

  py::class_<mpi::SpotEvent>(mpi, "SpotEvent")
      .def_readonly("time_ms", &mpi::SpotEvent::time_ms)
      .def_readonly("spotter_id", &mpi::SpotEvent::spotter_id, "uid of an aircraft, or 0x800 | uid of a ground vehicle")
      .def_readonly("spotted_id", &mpi::SpotEvent::spotted_id, "uid of an aircraft, or 0x800 | uid of a ground vehicle")
      .def_readonly("spotter", &mpi::SpotEvent::spotter)
      .def_readonly("spotted", &mpi::SpotEvent::spotted);

  py::class_<mpi::SpawnEvent>(mpi, "SpawnEvent")
      .def_readonly("time_ms", &mpi::SpawnEvent::time_ms, "Time of the type 6 ECS creation of the unit entity")
      .def_readonly("uid", &mpi::SpawnEvent::uid, "Unit id (low 11 bits); the kill record names the unit by it")
      .def_readonly("vehicle", &mpi::SpawnEvent::vehicle,
                    "unit__className, for example spitfire_f24, tankModels/fr_amx_m4, ships/us_sc_497")
      .def_readonly("player", &mpi::SpawnEvent::player, "Player slot (index into players); -1 for no player")
      .def_readonly("team", &mpi::SpawnEvent::team)
      .def_readonly("unit_type", &mpi::SpawnEvent::unit_type, "ECS unitType: 0 aircraft, 3 tank, 5 ship, 8 fortification")
      .def_readonly("mission_name", &mpi::SpawnEvent::mission_name, "Mission slot, for example t1_player05_0")
      .def_readonly("loadout", &mpi::SpawnEvent::loadout, "Loadout of the storage component; empty without a unit")
      .def_readonly("dummy", &mpi::SpawnEvent::dummy, "The dummy_plane of a player slot at battle start")
      .def_readonly("unit", &mpi::SpawnEvent::unit, "None for a fortification");

  py::class_<mpi::ShotEvent>(mpi, "ShotEvent")
      .def_readonly("time_ms", &mpi::ShotEvent::time_ms)
      .def_readonly("unit", &mpi::ShotEvent::unit)
      .def_readonly("kind", &mpi::ShotEvent::kind)
      .def_readonly("weapon_id", &mpi::ShotEvent::weapon_id);

  // The named fields are the ones whose meaning was checked (see SensorStates.h); the
  // rest go out raw, under the names of SensorsControlStates, so they can be studied.
  py::class_<mpi::SensorEvent>(mpi, "SensorEvent")
      .def_readonly("time_ms", &mpi::SensorEvent::time_ms)
      .def_readonly("unit", &mpi::SensorEvent::unit)
      .def_readonly("index", &mpi::SensorEvent::index)
      .def_readonly("list_tail", &mpi::SensorEvent::list_tail)
      .def_property_readonly("kind", [](const mpi::SensorEvent &e) { return e.state.kind(); },
                             "1 radar, IRST or optical tracker (the transceiver tells which), 2 laser")
      .def_property_readonly("slot", [](const mpi::SensorEvent &e) { return e.state.slot(); },
                             "Index of the sensor in the vehicle blk's sensors list")
      .def_property_readonly("on", [](const mpi::SensorEvent &e) { return e.state.on(); })
      .def_property_readonly("transceiver", [](const mpi::SensorEvent &e) { return e.state.transceiver(); },
                             "Index into the sensor blk's transivers, in key order; None if none")
      .def_property_readonly("scan_pattern", [](const mpi::SensorEvent &e) { return e.state.scan_pattern(); },
                             "Index into the sensor blk's scanPatterns, in key order; None if none")
      .def_property_readonly("signal", [](const mpi::SensorEvent &e) { return e.state.signal(); },
                             "Index into the sensor blk's signals, in key order; None if none")
      .def_property_readonly("mode_time", [](const mpi::SensorEvent &e) { return e.state.mode_time(); },
                             "Battle time in seconds of the last mode change")
      .def_property_readonly("scan_az", [](const mpi::SensorEvent &e) { return e.state.scan_az(); },
                             "Scan centre azimuth in radians, relative to the vehicle")
      .def_property_readonly("scan_el", [](const mpi::SensorEvent &e) { return e.state.scan_el(); },
                             "Scan centre elevation in radians, relative to the vehicle")
      .def_property_readonly("unknown_field_2", [](const mpi::SensorEvent &e) -> std::optional<float> {
        const auto &s = e.state;
        return ((s.kind() == 1 && s.has_state) || (s.kind() == 2 && s.on()) ||
                (s.kind() == 4 && s.has_kind4_state)) ? std::optional<float>(s.unknown_field_2) : std::nullopt;
      }, "Unknown field 2: float32; kind 1 is a normalized signed 16-bit value, kinds 2/4 send float32. "
         "None when the kind's data was absent; units and frame unknown")
      .def_property_readonly("unknown_field_3", [](const mpi::SensorEvent &e) -> std::optional<float> {
        const auto &s = e.state;
        return ((s.kind() == 2 && s.on()) || (s.kind() == 4 && s.has_kind4_state))
                 ? std::optional<float>(s.unknown_field_3) : std::nullopt;
      }, "Unknown field 3: float32 for kind 2 when on or kind 4 when its data is present; otherwise None. "
         "Units and frame unknown")
      .def_property_readonly("unknown_field_147", [](const mpi::SensorEvent &e) -> std::optional<float> {
        const auto &s = e.state;
        return s.kind() == 2 && s.on() ? std::optional<float>(s.unknown_field_147) : std::nullopt;
      }, "Unknown field 147: float32, kind 2 when on; otherwise None. Units and frame unknown")
      .def_property_readonly("unknown_field_6_data", [](const mpi::SensorEvent &e) -> py::object {
        const auto &s = e.state;
        if (s.kind() == 2 && s.on())
          return py::bytes(s.unknown_field_6_data, sizeof(s.unknown_field_6_data));
        if (s.kind() == 1 && s.has_state && (s.has_unknown_field_6_byte0 || s.has_unknown_fields_149_150))
          return py::bytes(s.unknown_field_6_data, sizeof(s.unknown_field_6_data));
        return py::none();
      }, "Unknown field 6: four raw bytes for kind 2 when on. For kind 1 with state, byte 0 is a raw "
         "8-bit value and byte 1 holds a decoded 1-bit value; either may be absent. Consult "
         "has_unknown_field_6_byte0 and has_unknown_fields_149_150. Other bytes are zero. "
         "None if no values were sent")
      .def_property_readonly("has_unknown_field_6_byte0", [](const mpi::SensorEvent &e) {
        return e.state.kind() == 1 && e.state.has_state && e.state.has_unknown_field_6_byte0;
      }, "Kind 1 field 6 byte 0 presence flag; false for other sensor kinds")
      .def_property_readonly("has_unknown_fields_149_150", [](const mpi::SensorEvent &e) {
        return e.state.kind() == 1 && e.state.has_state && e.state.has_unknown_fields_149_150;
      }, "Kind 1 presence flag for fields 149/150 and field 6 byte 1; false for other kinds")
      .def_property_readonly("unknown_field_149", [](const mpi::SensorEvent &e) -> std::optional<int> {
        return e.state.kind() == 1 && e.state.has_state && e.state.has_unknown_fields_149_150
                 ? std::optional<int>(e.state.unknown_field_149) : std::nullopt;
      }, "Unknown field 149: signed 2-bit value for kind 1 when its optional byte is present; otherwise None")
      .def_property_readonly("unknown_field_150", [](const mpi::SensorEvent &e) -> std::optional<int> {
        return e.state.kind() == 1 && e.state.has_state && e.state.has_unknown_fields_149_150
                 ? std::optional<int>(e.state.unknown_field_150) : std::nullopt;
      }, "Unknown field 150: signed 2-bit value for kind 1 when its optional byte is present; otherwise None")
      .def_property_readonly("contacts",
                             [](const mpi::SensorEvent &e) {
                               py::list out;
                               for (uint32_t c: e.state.contacts)
                                 out.append(c);
                               return out;
                             })
      .def_property_readonly("contacts_tail", [](const mpi::SensorEvent &e) { return e.state.contacts_tail; })
      .def_property_readonly("contact_uids", [](const mpi::SensorEvent &e) { return e.state.contact_uids(); },
                             "Unit uids of the targets the sensor detects at this sync");

  py::class_<mpi::DesignationEvent>(mpi, "DesignationEvent")
      .def_readonly("time_ms", &mpi::DesignationEvent::time_ms)
      .def_readonly("unit", &mpi::DesignationEvent::unit)
      .def_readonly("index", &mpi::DesignationEvent::index)
      .def_property_readonly("unknown_field_1", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_1; },
                             "Unknown field 1: unsigned 8-bit value, always present")
      .def_property_readonly("unknown_field_2", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_2; },
                             "Unknown field 2: unsigned 8-bit value, always present")
      .def_property_readonly("has_unknown_field_4", [](const mpi::DesignationEvent &e) { return e.state.has_unknown_field_4; },
                             "Field 3: 1-bit presence flag for unknown field 4")
      .def_property_readonly("unknown_field_4", [](const mpi::DesignationEvent &e) -> std::optional<Point3> {
        return e.state.has_unknown_field_4 ? std::optional<Point3>(e.state.unknown_field_4) : std::nullopt;
      }, "Unknown field 4: three float32 components if field 3 is set, otherwise None; units and frame unknown")
      .def_property_readonly("uses_compressed_vector", [](const mpi::DesignationEvent &e) { return e.state.uses_compressed_vector; },
                             "1-bit flag selecting fields 6/7 and field 9 encoding")
      .def_property_readonly("unknown_field_5", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_5; },
                             "Unknown field 5: float32, always present; units unknown")
      .def_property_readonly("unknown_field_6", [](const mpi::DesignationEvent &e) -> std::optional<Point3> {
        return e.state.uses_compressed_vector ? std::optional<Point3>(e.state.unknown_field_6) : std::nullopt;
      }, "Unknown field 6: three float32 components when uses_compressed_vector, otherwise None; units and frame unknown")
      .def_property_readonly("unknown_field_7", [](const mpi::DesignationEvent &e) -> std::optional<Point3> {
        return !e.state.uses_compressed_vector ? std::optional<Point3>(e.state.unknown_field_7) : std::nullopt;
      }, "Unknown field 7: three float32 components when not uses_compressed_vector, otherwise None; units and frame unknown")
      .def_property_readonly("unknown_field_8", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_8; },
                             "Unknown field 8: 1-bit boolean, always present")
      .def_property_readonly("unknown_field_9", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_9; },
                             "Unknown field 9: three decoded float32 components, always present. When "
                             "uses_compressed_vector, three signed 16-bit values scaled to 4000; otherwise three "
                             "float32 values. Units and frame unknown")
      .def_property_readonly("unknown_field_10", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_10; },
                             "Unknown field 10: 1-bit value, always present")
      .def_property_readonly("unknown_field_11", [](const mpi::DesignationEvent &e) -> std::optional<float> {
        return e.state.has_unknown_field_11 ? std::optional<float>(e.state.unknown_field_11) : std::nullopt;
      }, "Unknown field 11: optional unsigned 8-bit value normalized to float32 between 0 and 1; None when absent")
      .def_property_readonly("unknown_field_12", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_12; },
                             "Unknown field 12: 1-bit boolean, always present")
      .def_property_readonly("unknown_field_13", [](const mpi::DesignationEvent &e) { return e.state.unknown_field_13; },
                             "Unknown field 13: 1-bit boolean, always present")
      .def_property_readonly("unknown_field_14", [](const mpi::DesignationEvent &e) -> std::optional<uint8_t> {
        return e.state.has_unknown_field_14 ? std::optional<uint8_t>(e.state.unknown_field_14) : std::nullopt;
      }, "Unknown field 14: optional unsigned 8-bit value, None when absent")
      .def_property_readonly("unknown_field_15", [](const mpi::DesignationEvent &e) -> std::optional<uint32_t> {
        return e.state.has_unknown_field_15 ? std::optional<uint32_t>(e.state.unknown_field_15) : std::nullopt;
      }, "Unknown field 15: optional unsigned 32-bit value, None when absent; see target_uid for known IDs")
      .def_property_readonly("target_uid", [](const mpi::DesignationEvent &e) { return e.state.target_uid(); },
                             "Unit uid of the designated target, or None");

  py::class_<mpi::EngineSync>(mpi, "EngineSync")
      .def_readonly("state", &mpi::EngineSync::state, "7 while the engine runs, 8 once it has stopped")
      .def_readonly("afterburner", &mpi::EngineSync::afterburner,
                    "Afterburner throttle of a jet, 1.0 to 1.1; None at or below 100%, and always on a prop")
      .def_readonly("health", &mpi::EngineSync::health, "Engine health below 1.0, else None; 0 once stopped")
      .def_property_readonly(
        "radiators", [](const mpi::EngineSync &e) { return py::make_tuple(e.radiators[0], e.radiators[1]); },
        "Radiator flaps of a prop, 0 to 255 each; 0 on a jet");

  py::class_<mpi::ControlEvent>(mpi, "ControlEvent")
      .def_readonly("time_ms", &mpi::ControlEvent::time_ms)
      .def_readonly("unit", &mpi::ControlEvent::unit)
      .def_readonly("on_ground", &mpi::ControlEvent::on_ground)
      .def_property_readonly("pitch", &mpi::ControlEvent::pitch, "Pitch stick, -1 to 1, positive for a pull")
      .def_property_readonly("roll", &mpi::ControlEvent::roll, "Roll stick, -1 to 1")
      .def_property_readonly("rudder", &mpi::ControlEvent::rudder, "Rudder pedals, -1 to 1")
      .def_property_readonly("flaps", &mpi::ControlEvent::flaps, "0 to 1")
      .def_readonly("fire_mg", &mpi::ControlEvent::fire_mg, "Machine-gun trigger held")
      .def_readonly("fire_cannon", &mpi::ControlEvent::fire_cannon, "Cannon trigger held")
      .def_readonly("fire_additional", &mpi::ControlEvent::fire_additional, "Additional gun (gun pod) trigger held")
      .def_property_readonly("airbrake", &mpi::ControlEvent::airbrake, "0 to 1")
      .def_property_readonly("wheel_brake", &mpi::ControlEvent::wheel_brake, "0 to 1")
      .def_property_readonly("throttle", &mpi::ControlEvent::throttle,
                             "Throttle lever, 0 to 1; 1 also above 100%, see EngineSync.afterburner")
      .def_property_readonly("controls", [](const mpi::ControlEvent &e) {
        return py::bytes(reinterpret_cast<const char *>(e.controls), sizeof(e.controls));
      }, "The seven control bytes as sent")
      .def_readonly("engines", &mpi::ControlEvent::engines);

  py::class_<mpi::StoreSyncEvent>(mpi, "StoreSyncEvent")
      .def_readonly("time_ms", &mpi::StoreSyncEvent::time_ms)
      .def_readonly("eid", &mpi::StoreSyncEvent::eid)
      .def_readonly("server_time", &mpi::StoreSyncEvent::server_time,
                    "Server time in seconds, steps of 1/48 s; 0 to 42 ms after time_ms")
      .def_readonly("velocity", &mpi::StoreSyncEvent::velocity, "World velocity, m/s")
      .def_readonly("flight_time", &mpi::StoreSyncEvent::flight_time, "Seconds since launch, or None");

  using GW = mpi::GroundWeaponSync;
  py::class_<GW>(mpi, "GroundWeaponSync")
      .def_property_readonly("broken", &GW::broken,
                             "Probable: the weapon is disabled; rises at a hit, falls at the repair")
      .def_property_readonly("heat", &GW::heat,
                             "Probable: barrel heat, 0 to 1; about +0.2 per main-gun shot. None when not sent "
                             "(machine guns)")
      .def_property_readonly("turret_yaw", &GW::turret_yaw,
                             "Turret yaw relative to the hull, radians, the sign of the hull heading; None when "
                             "not sent")
      .def_property_readonly("turret_pitch", &GW::turret_pitch,
                             "Gun elevation relative to the hull, radians; None when not sent")
      .def_property_readonly("loaded_type", &GW::loaded_type,
                             "Index into the rounds of the ammunition in the gun: the type the next shot takes")
      .def_property_readonly("selected_type", &GW::selected_type,
                             "Index into the rounds of the ammunition chosen for the next load")
      .def_property_readonly("last_shot", &GW::last_shot, "Battle time in seconds of the last shot")
      .def_property_readonly("ready", &GW::ready,
                             "Raw ready field in seconds, or None if the ammo record was absent. Zero can occur "
                             "after a shot; its meaning and validity as a timestamp are unknown")
      .def_readonly("rounds_begin", &GW::rounds_begin, "First index in ParserState.ground_round_pool")
      .def_readonly("round_count", &GW::round_count,
                    "Rounds left per ammunition type, in ground_round_pool; 65535 is unlimited");

  using GS = mpi::GroundSyncEvent;
  py::class_<GS>(mpi, "GroundSyncEvent")
      .def_readonly("time_ms", &GS::time_ms)
      .def_readonly("uid", &GS::uid)
      .def_readonly("unit", &GS::unit, "None for an AI gun")
      .def_readonly("spawn_counter", &GS::spawn_counter,
                    "Two 4-bit spawn counters of the player; both rise at each new spawn")
      .def_property_readonly("has_motion", &GS::has_motion, "False when the motion values below were not sent")
      .def_property_readonly("is_ship", &GS::is_ship, "The ship block was sent; a ship sends no alive bit")
      .def_property_readonly("ship_throttle", &GS::ship_throttle, "127 stop, 254 full ahead, 0 full reverse")
      .def_property_readonly("ship_rudder", &GS::ship_rudder, "127 centre")
      .def_property_readonly("ship_flooding", &GS::ship_flooding, "Probable: the ship floods")
      .def_property_readonly("ship_water", [](const GS &e) {
        return py::make_tuple(e.ship_water[0], e.ship_water[1], e.ship_water[2]);
      }, "Probable: three water levels")
      .def_property_readonly("ship_shafts", &GS::ship_shafts,
                             "Health of each transmission_NN_dm part (propeller shaft), 255 intact")
      .def_readonly("velocity", &GS::velocity, "World axes, m/s")
      .def_property_readonly("angular_velocity", &GS::angular_velocity,
                             "Rad/s, in the order of the unit's euler: roll, yaw, pitch")
      .def_property_readonly("awake", &GS::awake, "Probable: the physics body is not at rest")
      .def_property_readonly("track_left", &GS::track_left, "Left track speed, m/s (measured scale)")
      .def_property_readonly("track_right", &GS::track_right, "Right track speed, m/s (measured scale)")
      .def_property_readonly("steer", &GS::steer, "Probable: steering input, -1 to 1")
      .def_property_readonly("throttle", &GS::throttle, "Drive input: 1 forward, 0, -1 reverse")
      .def_readonly("engine", &GS::engine, "Probable: engine speed, 0 to 255; 0 once stopped")
      .def_property_readonly("alive", &GS::alive, "Probable: the vehicle is alive; falls at the kill")
      .def_property_readonly("player_controlled", &GS::player_controlled,
                             "A player sits in the vehicle; falls about 10 s after the kill, False on AI guns")
      .def_property_readonly("aim_yaw", &GS::aim_yaw,
                             "Heading of the player's aim, radians, the sign of the hull heading; None without a "
                             "player")
      .def_property_readonly("aim_pitch", &GS::aim_pitch, "Probable: pitch of the aim, radians")
      .def_property_readonly("aim_distance", &GS::aim_distance,
                             "Probable: distance to the point under the aim, m; None when aim data is absent")
      .def_readonly("weapons_begin", &GS::weapons_begin, "First index in ParserState.ground_weapon_pool")
      .def_readonly("weapon_count", &GS::weapon_count, "Number of weapons in ParserState.ground_weapon_pool");

  py::enum_<mpi::SeekerSource>(mpi, "SeekerSource")
      .value("Weapon", mpi::SeekerWeapon)
      .value("Aircraft", mpi::SeekerAircraft)
      .value("Ground", mpi::SeekerGround);

  py::class_<mpi::SeekerEvent>(mpi, "SeekerEvent")
      .def_readonly("time_ms", &mpi::SeekerEvent::time_ms)
      .def_readonly("source", &mpi::SeekerEvent::source)
      .def_readonly("eid", &mpi::SeekerEvent::eid)
      .def_readonly("unit", &mpi::SeekerEvent::unit)
      .def_readonly("lost_for", &mpi::SeekerEvent::lost_for,
                    "Weapon only: seconds since the seeker lost its target; 0 while locked and before the first lock")
      .def_readonly("flight_time", &mpi::SeekerEvent::flight_time, "Weapon only: seconds since launch")
      .def_property_readonly("unknown_header_bit", [](const mpi::SeekerEvent &e) -> std::optional<bool> {
        return e.source == mpi::SeekerWeapon ? std::optional<bool>(e.unknown_header_bit) : std::nullopt;
      }, "Unknown 1-bit field preceding the block (formerly head_b); Weapon source only, otherwise None")
      .def_readonly("bits", &mpi::SeekerEvent::bits)
      .def_property_readonly("data", [](const mpi::SeekerEvent &e) {
        return py::bytes(reinterpret_cast<const char *>(e.data.data()), e.data.size());
      })
      .def_property_readonly("decoded", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).decoded; },
                             "True for the seeker blocks of a store in flight whose layout is known")
      .def_property_readonly("tracking", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).tracking; },
                             "True in track, False in search; None for an IR seeker or an unknown block")
      .def_property_readonly("los", [](const mpi::SeekerEvent &e) -> std::optional<Point3> {
        auto st = mpi::DecodeSeeker(e);
        return st.decoded ? std::optional<Point3>(st.los) : std::nullopt;
      }, "Unit line of sight from the missile, world axes")
      .def_property_readonly("range", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).range; },
                             "Radar seeker range in metres (0 to 15% above the true distance)")
      .def_property_readonly("target_pos", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).target_pos; },
                             "Radar seeker's estimate of the target position, world axes; for the 895-bit block "
                             "of an aircraft, the target the missile gets at launch")
      .def_property_readonly("ir_state", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).ir_state; },
                             "518-bit aircraft block: 0 no lock, 2 seeker on at the boresight, 4 axis on an aircraft, "
                             "3 probable: a target but the axis not on an aircraft")
      .def_property_readonly("lock_time", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).lock_time; },
                             "518-bit aircraft block: battle time of the last change out of state 0 or 2; a change "
                             "between 3 and 4 keeps it. None before the first")
      .def_property_readonly("seeker_dir", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).seeker_dir; },
                             "518-bit aircraft block: the seeker axis, unit vector in world axes")
      .def_property_readonly("gimbal_az", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).gimbal_az; },
                             "518-bit aircraft block: gimbal azimuth in radians, positive to the left; 0 while caged")
      .def_property_readonly("gimbal_el", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).gimbal_el; },
                             "518-bit aircraft block: gimbal elevation in radians, positive up; 0 while caged")
      .def_property_readonly("fov_half", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).fov_half; },
                             "518-bit aircraft block: half the seeker fov in radians");

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SeekerEvent>>(m, "SeekerEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::ControlEvent>>(m, "ControlEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::GroundSyncEvent>>(m, "GroundSyncEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::GroundWeaponSync>>(m, "GroundWeaponSyncList");
  bind_readonly_vector<std::pmr::vector<uint16_t>>(m, "GroundRoundList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::StoreSyncEvent>>(m, "StoreSyncEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SensorEvent>>(m, "SensorEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::DesignationEvent>>(m, "DesignationEventList");

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitEffect>>(m, "HitEffectList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitAnalysis>>(m, "HitAnalysisList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDamage>>(m, "HitDamageList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDirection>>(m, "HitDirectionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitExplosion>>(m, "HitExplosionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::ShotEvent>>(m, "ShotEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SpotEvent>>(m, "SpotEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SpawnEvent>>(m, "SpawnEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::CockpitEvent>>(m, "CockpitEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::RpmEvent>>(m, "RpmEventList");

  py::class_<mpi::AwardMessage, mpi::IBattleMessage, std::unique_ptr<mpi::AwardMessage, py::nodelete>>(mpi,
                                                                                                       "AwardMessage")
      .def_readonly("player_pid", &mpi::AwardMessage::player_pid)
      .def_readonly("award", &mpi::AwardMessage::award)
      .def_readonly("stage", &mpi::AwardMessage::stage)
      .def_readonly("wp", &mpi::AwardMessage::wp)
      .def_readonly("exp", &mpi::AwardMessage::exp);
}
