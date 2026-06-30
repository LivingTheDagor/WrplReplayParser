#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_4(py::module &gen);
void include_types_3(py::module &gen) {
  // danet::ReflectionVar<std::array<ecs::EntityId,20>> bindings
  py::class_<danet::ReflectionVar<std::array<ecs::EntityId, 20>>::SpaceHandler::TimeState>(
    gen, "std_array_ecs_EntityId_20__ts")
    .def_readonly("time_ms", &danet::ReflectionVar<std::array<ecs::EntityId, 20>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::array<ecs::EntityId, 20>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::array<ecs::EntityId, 20>>::SpaceHandler::TimeState>>(
    gen, "std_array_ecs_EntityId_20__ts_vector");

  py::class_<danet::ReflectionVar<std::array<ecs::EntityId,20>>>(gen, "std_array_ecs_EntityId_20__var")
  .def_readonly("data", &danet::ReflectionVar<std::array<ecs::EntityId,20>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::array<ecs::EntityId,20>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::CrewUnitsList> bindings
  py::class_<danet::ReflectionVar<danet::CrewUnitsList>::SpaceHandler::TimeState>(gen, "danet_CrewUnitsList_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<danet::CrewUnitsList>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<danet::CrewUnitsList>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::CrewUnitsList>::SpaceHandler::TimeState>>(
    gen, "danet_CrewUnitsList_ts_vector");

  py::class_<danet::ReflectionVar<danet::CrewUnitsList>>(gen, "danet_CrewUnitsList_var")
  .def_readonly("data", &danet::ReflectionVar<danet::CrewUnitsList>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::CrewUnitsList> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<std::vector<uint8_t>> bindings
  py::class_<danet::ReflectionVar<std::vector<uint8_t>>::SpaceHandler::TimeState>(gen, "std_vector_uint8_t__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<uint8_t>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<uint8_t>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint8_t>>::SpaceHandler::TimeState>>(
    gen, "std_vector_uint8_t__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<uint8_t>>>(gen, "std_vector_uint8_t__var")
  .def_readonly("data", &danet::ReflectionVar<std::vector<uint8_t>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<uint8_t>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<std::vector<uint16_t>> bindings
  py::class_<danet::ReflectionVar<std::vector<uint16_t>>::SpaceHandler::TimeState>(gen, "std_vector_uint16_t__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<uint16_t>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<uint16_t>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint16_t>>::SpaceHandler::TimeState>>(
    gen, "std_vector_uint16_t__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<uint16_t>>>(gen, "std_vector_uint16_t__var")
  .def_readonly("data", &danet::ReflectionVar<std::vector<uint16_t>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<uint16_t>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::dummyForKillStreaksProgress> bindings
  py::class_<danet::ReflectionVar<danet::dummyForKillStreaksProgress>::SpaceHandler::TimeState>(gen, "danet_dummyForKillStreaksProgress_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForKillStreaksProgress>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForKillStreaksProgress>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForKillStreaksProgress>::SpaceHandler::TimeState>>(
    gen, "danet_dummyForKillStreaksProgress_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForKillStreaksProgress>>(gen, "danet_dummyForKillStreaksProgress_var")
    .def_readonly("data", &danet::ReflectionVar<danet::dummyForKillStreaksProgress>::data)
    .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForKillStreaksProgress> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::RoundScore> bindings
  py::class_<danet::ReflectionVar<danet::RoundScore>::SpaceHandler::TimeState>(gen, "danet_RoundScore_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::RoundScore>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::RoundScore>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::RoundScore>::SpaceHandler::TimeState>>(
    gen, "danet_RoundScore_ts_vector");

  py::class_<danet::ReflectionVar<danet::RoundScore>>(gen, "danet_RoundScore_var")
    .def_readonly("data", &danet::ReflectionVar<danet::RoundScore>::data)
    .def_property_readonly("history", [](danet::ReflectionVar<danet::RoundScore> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::dummyForPlayerStat> bindings
  py::class_<danet::ReflectionVar<danet::dummyForPlayerStat>::SpaceHandler::TimeState>(gen, "danet_dummyForPlayerStat_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForPlayerStat>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForPlayerStat>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForPlayerStat>::SpaceHandler::TimeState>>(
    gen, "danet_dummyForPlayerStat_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForPlayerStat>>(gen, "danet_dummyForPlayerStat_var")
    .def_readonly("data", &danet::ReflectionVar<danet::dummyForPlayerStat>::data)
    .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForPlayerStat> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::dummyForFootballStat> bindings
  py::class_<danet::ReflectionVar<danet::dummyForFootballStat>::SpaceHandler::TimeState>(gen, "danet_dummyForFootballStat_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForFootballStat>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForFootballStat>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForFootballStat>::SpaceHandler::TimeState>>(
    gen, "danet_dummyForFootballStat_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForFootballStat>>(gen, "danet_dummyForFootballStat_var")
    .def_readonly("data", &danet::ReflectionVar<danet::dummyForFootballStat>::data)
    .def_property_readonly(
      "history", [](danet::ReflectionVar<danet::dummyForFootballStat> &self) { return &self.get_history(); },
      py::return_value_policy::reference_internal);


  include_types_4(gen);
}