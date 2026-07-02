#include "modules/mpi/unit.h"
#include "Unit.h"
#include "modules/bind_readonly_vector.h"
#include "modules/mpi/codegen_objects.h"
#include "mpi/types.h"
#include "state/ParserState.h"


PyUnit py_unit;

std::vector<unit::Unit *> collect_all_units(ParserState &state);

std::vector<Rocket *> collect_all_rockets(ParserState &state);

std::vector<Bomb *> collect_all_bombs(ParserState &state);

void PyUnit::include(py::module_ &m) {
  DO_INCLUDE()
  auto unit = m.def_submodule("unit");
  py::class_<SpaceTime>(m, "SpaceTime")
    .def_readonly("time_ms", &SpaceTime::time_ms)
    .def_readonly("location", &SpaceTime::location)
    .def("__str__", [](SpaceTime &st) {
      return fmt::format("SpaceTime({}, [{}, {}, {}])", st.time_ms, st.location.x, st.location.y, st.location.z);
    });

  bind_readonly_vector<std::vector<SpaceTime>>(m, "SpaceTimeList");

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

  py::class_<unit::Weapon>(unit, "Weapon")
    .def_readonly("weapon_id", &unit::Weapon::weapon_id)
    .def_readonly("weapon_index", &unit::Weapon::weapon_index)
    .def_readonly("emitter", &unit::Weapon::emitter)
    .def_readonly("blk_path", &unit::Weapon::blk_path)
    .def_readonly("weapon_name", &unit::Weapon::weapon_name);

  un.def_readonly("base_data", &unit::Unit::base_data)
    .def_readonly("base_dvm_data", &unit::Unit::base_dvm_data)
    .def_readonly("unitType", &unit::Unit::unitType)
    .def_readonly("uid", &unit::Unit::uid)
    .def_readonly("created_at_ms", &unit::Unit::created_at_ms)
    .def_readonly("killed_at_ms", &unit::Unit::killed_at_ms)
    .def_readonly("killed_position", &unit::Unit::killed_position)
    .def_readonly("destroyed_at_ms", &unit::Unit::destroyed_at_ms)
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

  py::class_<unit::UnitRef>(unit, "UnitRef").def_readonly("unit", &unit::UnitRef::unit);

  py::class_<Rocket>(unit, "Rocket")
    .def_readonly("positions", &Rocket::positions)
    .def_readonly("created_at_ms", &Rocket::created_at_ms)
    .def_readonly("destroyed_at_ms", &Rocket::destroyed_at_ms)
    .def_readonly("ownerEid", &Rocket::ownerEid)
    .def_readonly("eid2", &Rocket::eid2);

  py::class_<Bomb, Rocket>(unit, "Bomb");
  py::class_<Torpedo, Rocket>(unit, "Torpedo");
  py::class_<Jettisoned, Rocket>(unit, "Jettisoned");
  py::class_<Payload, Rocket>(unit, "Payload");

  bind_readonly_vector<std::vector<unit::Unit *>>(m, "UnitList");
  m.def("collect_all_units", &collect_all_units, py::arg("state"));
  m.def("collect_all_rockets", &collect_all_rockets, py::arg("state"));
  m.def("collect_all_bombs", &collect_all_bombs, py::arg("state"));
}
