#include "modules/mpi/battle_messages.h"
#include "Unit.h"
#include "modules/ecs/EntityId.h"
#include "mpi/GeneralObject.h"
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

  py::class_<mpi::ShotEvent>(mpi, "ShotEvent")
      .def_readonly("time_ms", &mpi::ShotEvent::time_ms)
      .def_readonly("unit", &mpi::ShotEvent::unit)
      .def_readonly("kind", &mpi::ShotEvent::kind)
      .def_readonly("weapon_id", &mpi::ShotEvent::weapon_id);

  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitEffect>>(m, "HitEffectList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitAnalysis>>(m, "HitAnalysisList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDamage>>(m, "HitDamageList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitDirection>>(m, "HitDirectionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::HitExplosion>>(m, "HitExplosionList");
  bind_readonly_vector_no_contain<std::pmr::vector<mpi::ShotEvent>>(m, "ShotEventList");

  py::class_<mpi::AwardMessage, mpi::IBattleMessage, std::unique_ptr<mpi::AwardMessage, py::nodelete>>(mpi,
                                                                                                       "AwardMessage")
      .def_readonly("player_pid", &mpi::AwardMessage::player_pid)
      .def_readonly("award", &mpi::AwardMessage::award)
      .def_readonly("stage", &mpi::AwardMessage::stage)
      .def_readonly("wp", &mpi::AwardMessage::wp)
      .def_readonly("exp", &mpi::AwardMessage::exp);
}
