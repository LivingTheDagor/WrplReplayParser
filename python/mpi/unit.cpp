#include "modules/mpi/unit.h"
#include "Unit.h"
#include "modules/bind_readonly_vector.h"
#include "modules/bind_rewind_state.h"
#include "modules/mpi/codegen_objects.h"
#include "mpi/types.h"
#include "state/ParserState.h"


PyUnit py_unit;

void PyUnit::include(py::module_ &m) {
  DO_INCLUDE()
  auto unit = m.def_submodule("unit");
  py::class_<SpaceTime>(unit, "SpaceTime")
    .def_readonly("location", &SpaceTime::location)
    .def("__str__", [](SpaceTime &st) {
      return fmt::format("SpaceTime([{}, {}, {}])", st.location.x, st.location.y, st.location.z);
    });

  py::class_<SpaceTimeEuler, SpaceTime>(unit, "SpaceTimeEuler")
    .def_readonly("euler", &SpaceTimeEuler::euler)
    .def("__str__", [](SpaceTimeEuler &st) {
      return fmt::format("SpaceTimeEuler([{}, {}, {}], [{}, {}, {}])", st.location.x, st.location.y, st.location.z,
                         st.euler.x, st.euler.y, st.euler.z);
    });


  py::class_<ObjectRewindState<SpaceTimeEuler, false, false, false>::TimeState>(m, "SpaceTimeEulerTS")
    .def_readonly("time_ms", &ObjectRewindState<SpaceTimeEuler, false, false, false>::TimeState::time_ms)
    .def_readonly("value", &ObjectRewindState<SpaceTimeEuler, false, false, false>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<SpaceTimeEuler, false, false, false>::TimeState>>(
    m, "SpaceTimeEulerTSList");


  bind_rewind_state<ObjectRewindState<SpaceTimeEuler, false, false, false>>(m, "SpaceTimeEulerHistory")
    .def("channels", [](py::object self_py) {
      auto &self = self_py.cast<ObjectRewindState<SpaceTimeEuler, false, false, false> &>();
      auto &v = self.history();
      py::dict d;
      if (v.empty())
        return d;
      const auto *b = v.data();
      const size_t n = v.size();
      d["time_ms"] = channel_view(b, &b->time_ms, n, self_py);
      d["x"] = channel_view(b, &b->data.location.x, n, self_py);
      d["y"] = channel_view(b, &b->data.location.y, n, self_py);
      d["z"] = channel_view(b, &b->data.location.z, n, self_py);
      d["yaw"] = channel_view(b, &b->data.euler.y, n, self_py);
      d["pitch"] = channel_view(b, &b->data.euler.z, n, self_py);
      d["roll"] = channel_view(b, &b->data.euler.x, n, self_py);
      return d;
    },
         "All samples as read-only zero-copy numpy views over the history buffer.\n"
         "Empty dict when the history is empty, otherwise every array has the same length.\n"
         "Keys: time_ms (uint32, ms); x, y, z (float32, meters, y is altitude);\n"
         "yaw, pitch, roll (float32, radians, hull orientation).");

  bind_readonly_vector<std::vector<SpaceTime>>(m, "SpaceTimeList");
  bind_readonly_vector<std::vector<SpaceTimeEuler>>(m, "SpaceTimeEulerList");

  py::enum_<UnitType>(unit, "UnitType")
    .value("TankType", UnitType::TankType)
    .value("AircraftType", UnitType::AircraftType);

  py::class_<unit::weapon_data>(unit, "WeaponData")
    .def_readonly("launcher", &unit::weapon_data::launcher)
    .def_readonly("bullet", &unit::weapon_data::bullet)
    .def_readonly("count", &unit::weapon_data::count);
  py::class_<unit::Unit, std::unique_ptr<unit::Unit, py::nodelete>> un(unit, "Unit");


  py_codegen_objects.include(m); // codegen objects require unit::Unit to exist

  py::class_<unit::Aircraft, unit::Unit, std::unique_ptr<unit::Aircraft, py::nodelete>>(unit, "Aircraft");

  py::class_<unit::Tank, unit::Unit, std::unique_ptr<unit::Tank, py::nodelete>> t(unit, "Tank");

  py::class_<unit::TurretData>(m, "TurretData")
    .def_readonly("rel", &unit::TurretData::rel)
    .def_readonly("abs", &unit::TurretData::abs)
    .def_readonly("gun_rel", &unit::TurretData::gun_rel)
    .def_readonly("gun_abs", &unit::TurretData::gun_abs);

  py::class_<ObjectRewindState<unit::TurretData, false, false, false>::TimeState>(m, "TurretDataTS")
    .def_readonly("time_ms", &ObjectRewindState<unit::TurretData, false, false, false>::TimeState::time_ms)
    .def_readonly("value", &ObjectRewindState<unit::TurretData, false, false, false>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<unit::TurretData, false, false, false>::TimeState>>(
    m, "TurretDataTSList");


  bind_rewind_state<ObjectRewindState<unit::TurretData, false, false, false>>(m, "TurretDataHistory")
    .def("channels", [](py::object self_py) {
      auto &self = self_py.cast<ObjectRewindState<unit::TurretData, false, false, false> &>();
      auto &v = self.history();
      py::dict d;
      if (v.empty())
        return d;
      const auto *b = v.data();
      const size_t n = v.size();
      d["time_ms"] = channel_view(b, &b->time_ms, n, self_py);
      d["yaw"] = channel_view(b, &b->data.abs.x, n, self_py);
      d["pitch"] = channel_view(b, &b->data.abs.y, n, self_py);
      d["roll"] = channel_view(b, &b->data.abs.z, n, self_py);
      d["rel_yaw"] = channel_view(b, &b->data.rel.x, n, self_py);
      d["rel_pitch"] = channel_view(b, &b->data.rel.y, n, self_py);
      d["rel_roll"] = channel_view(b, &b->data.rel.z, n, self_py);
      d["gun_yaw"] = channel_view(b, &b->data.gun_abs.x, n, self_py);
      d["gun_pitch"] = channel_view(b, &b->data.gun_abs.y, n, self_py);
      d["gun_roll"] = channel_view(b, &b->data.gun_abs.z, n, self_py);
      d["gun_rel_yaw"] = channel_view(b, &b->data.gun_rel.x, n, self_py);
      d["gun_rel_pitch"] = channel_view(b, &b->data.gun_rel.y, n, self_py);
      d["gun_rel_roll"] = channel_view(b, &b->data.gun_rel.z, n, self_py);
      return d;
    },
         "All samples as read-only zero-copy numpy views over the history buffer.\n"
         "Empty dict when the history is empty, otherwise every array has the same length.\n"
         "Keys: time_ms (uint32, ms); then float32 radians, in yaw/pitch/roll order:\n"
         "yaw, pitch, roll - turret node (head), absolute;\n"
         "rel_yaw, rel_pitch, rel_roll - turret node, relative to its parent node;\n"
         "gun_yaw, gun_pitch, gun_roll - gun node, absolute;\n"
         "gun_rel_yaw, gun_rel_pitch, gun_rel_roll - gun node, relative to the turret.\n"
         "Absolute values are the sum of the parent absolute and the relative one, per axis,\n"
         "with the hull as the tree root: turret traverse = yaw - hull yaw,\n"
         "gun elevation = gun_pitch - pitch. Only yaw, pitch and gun_pitch carry new data,\n"
         "the rest duplicate the hull or the turret.");


  py::class_<unit::TurretDesc, std::unique_ptr<unit::TurretDesc, py::nodelete>>(unit, "TurretDesc")
    .def_readonly("turret_state", &unit::TurretDesc::turret_state);

  py::class_<unit::Weapon>(unit, "Weapon")
    .def_readonly("weapon_id", &unit::Weapon::weapon_id)
    .def_readonly("weapon_index", &unit::Weapon::weapon_index)
    .def_readonly("emitter", &unit::Weapon::emitter)
    .def_readonly("blk_path", &unit::Weapon::blk_path)
    .def_readonly("weapon_name", &unit::Weapon::weapon_name)
    .def_readonly("name_index", &unit::Weapon::name_index)
    .def_readonly("name_index_short", &unit::Weapon::name_index_short)
    .def_property_readonly(
      "turret", [](unit::Weapon &self) { return self.turret_desc.get(); }, py::return_value_policy::reference_internal);

  un.def_readonly("base_data", &unit::Unit::base_data)
    .def_readonly("base_dvm_data", &unit::Unit::base_dvm_data)
    .def_readonly("unitType", &unit::Unit::unitType)
    .def_readonly("uid", &unit::Unit::uid)
    .def_readonly("created_at_ms", &unit::Unit::created_at_ms, "When the unit was created in the ECS. does not correlate to when it actually spawned in")
    .def_readonly("killed_at_ms", &unit::Unit::killed_at_ms,
                  "When the vehicle was killed.\n of 0xFFFFFFFF, then the unit is still alive")
    .def_readonly("killed_position", &unit::Unit::killed_position)
    .def_readonly("destroyed_at_ms", &unit::Unit::destroyed_at_ms,
                  "When the entity was destroyed in the ECS, Distinct from killed_at_ms, controlled by server\n"
                  "if 0xFFFFFFFF, then that means the unit or its corpse exists somewhere in the world")
    .def("AsAircraft", &unit::Unit::AsAircraft)
    .def("AsTank", &unit::Unit::AsTank)
    .def_readonly("unit_name", &unit::Unit::raw_unit_name) // mr luxman decided to be lazy
    .def_readonly("unit_name_clean", &unit::Unit::unit_name)
    .def_readonly("player_internal_name", &unit::Unit::player_internal_name)
    .def_readonly("owner_pid", &unit::Unit::owner_pid)
    .def_readonly("spawn_position", &unit::Unit::spawn_position)
    .def_readonly("loadout_name", &unit::Unit::loadout_name)
    .def_readonly("skin_name", &unit::Unit::skin_name)
    .def_readonly("camo_info", &unit::Unit::camo_info)
    .def_readonly("custom_weapons_blk", &unit::Unit::custom_weapons_blk)
    .def_readonly("weapons", &unit::Unit::storage_weapons)
    .def_readonly("weapon_mods", &unit::Unit::weapon_mods)
    .def_readonly("actual_weapons", &unit::Unit::weapons)
    .def_readonly("fm_mods", &unit::Unit::fm_mods)
    .def_readonly("positions", &unit::Unit::positions)
    .def_readonly("unit_wpcost", &unit::Unit::unit_wpcost)
    .def_readonly("unit_tags", &unit::Unit::unit_tags)
    .def("getTags", &unit::Unit::getTags)
    .def_readonly("name_index_shop", &unit::Unit::name_index_shop)
    .def_readonly("name_index_0", &unit::Unit::name_index_0)
    .def_readonly("name_index_1", &unit::Unit::name_index_1)
    .def_readonly("name_index_2", &unit::Unit::name_index_2);

  unit.def("getUnitTagsName", &unit::getUnitTagsName, py::arg("name"));
  unit.def("getUnitTagsBlk", &unit::getUnitTagsBlk, py::arg("blk"));

  py::class_<unit::UnitRef>(unit, "UnitRef").def_readonly("unit", &unit::UnitRef::unit);

  py::class_<Rocket, std::unique_ptr<Rocket, py::nodelete>>(unit, "Rocket")
    .def_readonly("positions", &Rocket::positions)
    .def_readonly("created_at_ms", &Rocket::created_at_ms)
    .def_readonly("destroyed_at_ms", &Rocket::destroyed_at_ms)
    .def_readonly("ownerEid", &Rocket::ownerEid,
                  "Entity id of the shooter. Resolving it through the entity manager fails once the\n"
                  "entity is gone from the world, so prefer owned_by.")
    .def_readonly("eid2", &Rocket::eid2, "Always 0:0 on every replay checked so far. Do not rely on it.")
    .def_readonly("weapon_obj", &Rocket::weapon_obj,
                  "The launcher this projectile came from, or None. Empty for aircraft twin mounts,\n"
                  "where weapon_name is still correct.")
    .def_readonly("owned_by", &Rocket::owned_by,
                  "The unit that fired this projectile. Stronger than ownerEid: it resolves even when\n"
                  "the shooter entity has already been removed from the world.");

  py::class_<Bomb, Rocket, std::unique_ptr<Bomb, py::nodelete>>(unit, "Bomb");
  py::class_<Torpedo, Rocket, std::unique_ptr<Torpedo, py::nodelete>>(unit, "Torpedo");
  py::class_<Jettisoned, Rocket, std::unique_ptr<Jettisoned, py::nodelete>>(unit, "Jettisoned");
  py::class_<Payload, Rocket, std::unique_ptr<Payload, py::nodelete>>(unit, "Payload");

  bind_readonly_vector<std::vector<unit::Unit *>>(m, "UnitList");
}
