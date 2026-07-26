#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_3(py::module &gen);
void include_types_2(py::module &gen) {
  //danet::ReflectionVar<std::vector<danet::WeatherEffect>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::TimeState>(gen, "std_vector_danet_WeatherEffect__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::WeatherEffect>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<danet::WeatherEffect>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::TimeState>>(gen, "std_vector_danet_WeatherEffect__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::WeatherEffect>>>(gen, "std_vector_danet_WeatherEffect__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<danet::WeatherEffect>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<danet::WeatherEffect>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::WeatherEffect>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::UnitId> bindings
  py::class_<danet::ReflectionVar<danet::UnitId>::TimeState>(gen, "danet_UnitId_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::UnitId>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::UnitId>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::UnitId>::TimeState>>(gen, "danet_UnitId_ts_vector");

  py::class_<danet::ReflectionVar<danet::UnitId>>(gen, "danet_UnitId_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::UnitId> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::UnitId> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::UnitId> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<uint64_t> bindings
  py::class_<danet::ReflectionVar<uint64_t>::TimeState>(gen, "uint64_t_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<uint64_t>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<uint64_t>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint64_t>::TimeState>>(gen, "uint64_t_ts_vector");

  py::class_<danet::ReflectionVar<uint64_t>>(gen, "uint64_t_var")
  .def_property_readonly("data", [](danet::ReflectionVar<uint64_t> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<uint64_t> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<uint64_t> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::Uid> bindings
  py::class_<danet::ReflectionVar<danet::Uid>::TimeState>(gen, "danet_Uid_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::Uid>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::Uid>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Uid>::TimeState>>(gen, "danet_Uid_ts_vector");

  py::class_<danet::ReflectionVar<danet::Uid>>(gen, "danet_Uid_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::Uid> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::Uid> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::Uid> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<DataBlock> bindings
  py::class_<danet::ReflectionVar<DataBlock>::TimeState>(gen, "DataBlock_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<DataBlock>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<DataBlock>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<DataBlock>::TimeState>>(gen, "DataBlock_ts_vector");

  py::class_<danet::ReflectionVar<DataBlock>>(gen, "DataBlock_var")
  .def_property_readonly("data", [](danet::ReflectionVar<DataBlock> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<DataBlock> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<DataBlock> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::Country> bindings
  py::class_<danet::ReflectionVar<danet::Country>::TimeState>(gen, "danet_Country_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::Country>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::Country>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Country>::TimeState>>(gen, "danet_Country_ts_vector");

  py::class_<danet::ReflectionVar<danet::Country>>(gen, "danet_Country_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::Country> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::Country> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::Country> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::array<ecs::EntityId,20>> bindings
  py::class_<danet::ReflectionVar<std::array<ecs::EntityId,20>>::TimeState>(gen, "std_array_ecs_EntityId_20__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::array<ecs::EntityId,20>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::array<ecs::EntityId,20>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::array<ecs::EntityId,20>>::TimeState>>(gen, "std_array_ecs_EntityId_20__ts_vector");

  py::class_<danet::ReflectionVar<std::array<ecs::EntityId,20>>>(gen, "std_array_ecs_EntityId_20__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::array<ecs::EntityId,20>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::array<ecs::EntityId,20>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::array<ecs::EntityId,20>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::CrewUnitsList> bindings
  py::class_<danet::ReflectionVar<danet::CrewUnitsList>::TimeState>(gen, "danet_CrewUnitsList_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::CrewUnitsList>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::CrewUnitsList>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::CrewUnitsList>::TimeState>>(gen, "danet_CrewUnitsList_ts_vector");

  py::class_<danet::ReflectionVar<danet::CrewUnitsList>>(gen, "danet_CrewUnitsList_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::CrewUnitsList> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::CrewUnitsList> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::CrewUnitsList> &self){return &self.history();}, py::return_value_policy::reference_internal);


  include_types_3(gen);
}