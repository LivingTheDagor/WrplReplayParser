#include "modules/mpi/battle_messages.h"
#include "Unit.h"
#include "modules/ecs/EntityId.h"
#include "mpi/GeneralObject.h"
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

  py::class_<mpi::SpotEvent>(mpi, "SpotEvent")
      .def_readonly("time_ms", &mpi::SpotEvent::time_ms)
      .def_readonly("spotter_id", &mpi::SpotEvent::spotter_id, "uid of an aircraft, or 0x800 | uid of a ground vehicle")
      .def_readonly("spotted_id", &mpi::SpotEvent::spotted_id, "uid of an aircraft, or 0x800 | uid of a ground vehicle")
      .def_readonly("spotter", &mpi::SpotEvent::spotter)
      .def_readonly("spotted", &mpi::SpotEvent::spotted);

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
      .def_property_readonly("f2", [](const mpi::SensorEvent &e) { return e.state.some_data_2; })
      .def_property_readonly("f3", [](const mpi::SensorEvent &e) { return e.state.some_data_3; })
      .def_property_readonly("f147", [](const mpi::SensorEvent &e) { return e.state.field147_0xa8; })
      .def_property_readonly("b6", [](const mpi::SensorEvent &e) {
        return py::bytes(e.state.some_data_6, sizeof(e.state.some_data_6));
      })
      .def_property_readonly("i149", [](const mpi::SensorEvent &e) { return e.state.field149_0xa4; })
      .def_property_readonly("i150", [](const mpi::SensorEvent &e) { return e.state.field150_0xa8; })
      .def_property_readonly("contacts",
                             [](const mpi::SensorEvent &e) {
                               py::list out;
                               for (uint32_t c: e.state.field4_0x4)
                                 out.append(c);
                               return out;
                             })
      .def_property_readonly("contacts_tail", [](const mpi::SensorEvent &e) { return e.state.field133_0x85; })
      .def_property_readonly("contact_uids", [](const mpi::SensorEvent &e) { return e.state.contact_uids(); },
                             "Unit uids of the targets the sensor detects at this sync");

  py::class_<mpi::DesignationEvent>(mpi, "DesignationEvent")
      .def_readonly("time_ms", &mpi::DesignationEvent::time_ms)
      .def_readonly("unit", &mpi::DesignationEvent::unit)
      .def_readonly("index", &mpi::DesignationEvent::index)
      .def_property_readonly("v1", [](const mpi::DesignationEvent &e) { return e.state.v1; })
      .def_property_readonly("v2", [](const mpi::DesignationEvent &e) { return e.state.v2; })
      .def_property_readonly("v3", [](const mpi::DesignationEvent &e) { return e.state.v3; })
      .def_property_readonly("v4", [](const mpi::DesignationEvent &e) { return e.state.v4; })
      .def_property_readonly("compressed", [](const mpi::DesignationEvent &e) { return e.state.write_compressed; })
      .def_property_readonly("v5", [](const mpi::DesignationEvent &e) { return e.state.v5; })
      .def_property_readonly("v6", [](const mpi::DesignationEvent &e) { return e.state.v6; })
      .def_property_readonly("v7", [](const mpi::DesignationEvent &e) { return e.state.v7; })
      .def_property_readonly("v8", [](const mpi::DesignationEvent &e) { return e.state.v8; })
      .def_property_readonly("v9", [](const mpi::DesignationEvent &e) { return e.state.v9; })
      .def_property_readonly("v10", [](const mpi::DesignationEvent &e) { return e.state.v10; })
      .def_property_readonly("v11", [](const mpi::DesignationEvent &e) { return e.state.v11; })
      .def_property_readonly("v12", [](const mpi::DesignationEvent &e) { return e.state.v12; })
      .def_property_readonly("v13", [](const mpi::DesignationEvent &e) { return e.state.v13; })
      .def_property_readonly("v14", [](const mpi::DesignationEvent &e) { return e.state.v14; })
      .def_property_readonly("v15", [](const mpi::DesignationEvent &e) { return e.state.v15; })
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
      .def_property_readonly("airbrake", &mpi::ControlEvent::airbrake, "0 to 1")
      .def_property_readonly("wheel_brake", &mpi::ControlEvent::wheel_brake, "0 to 1")
      .def_property_readonly("throttle", &mpi::ControlEvent::throttle,
                             "Throttle lever, 0 to 1; 1 also above 100%, see EngineSync.afterburner")
      .def_property_readonly("controls", [](const mpi::ControlEvent &e) {
        return py::bytes(reinterpret_cast<const char *>(e.controls), sizeof(e.controls));
      }, "The seven control bytes as sent")
      .def_readonly("engines", &mpi::ControlEvent::engines);

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
      .def_readonly("head_b", &mpi::SeekerEvent::head_b)
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
                             "518-bit aircraft block: 0 no lock, 2 a new lock, 3 and 4 lock held")
      .def_property_readonly("lock_time", [](const mpi::SeekerEvent &e) { return mpi::DecodeSeeker(e).lock_time; },
                             "518-bit aircraft block: battle time in seconds of the last lock, None before the first");

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SeekerEvent>>(m, "SeekerEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::ControlEvent>>(m, "ControlEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SensorEvent>>(m, "SensorEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::DesignationEvent>>(m, "DesignationEventList");

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitEffect>>(m, "HitEffectList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitAnalysis>>(m, "HitAnalysisList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDamage>>(m, "HitDamageList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDirection>>(m, "HitDirectionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitExplosion>>(m, "HitExplosionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::ShotEvent>>(m, "ShotEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::SpotEvent>>(m, "SpotEventList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::CockpitEvent>>(m, "CockpitEventList");

  py::class_<mpi::AwardMessage, mpi::IBattleMessage, std::unique_ptr<mpi::AwardMessage, py::nodelete>>(mpi,
                                                                                                       "AwardMessage")
      .def_readonly("player_pid", &mpi::AwardMessage::player_pid)
      .def_readonly("award", &mpi::AwardMessage::award)
      .def_readonly("stage", &mpi::AwardMessage::stage)
      .def_readonly("wp", &mpi::AwardMessage::wp)
      .def_readonly("exp", &mpi::AwardMessage::exp);
}
