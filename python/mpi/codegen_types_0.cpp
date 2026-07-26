#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_1(py::module &gen);
void include_types_0(py::module &gen) {
  //danet::ReflectionVar<uint32_t> bindings
  py::class_<danet::ReflectionVar<uint32_t>::TimeState>(gen, "uint32_t_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<uint32_t>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<uint32_t>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint32_t>::TimeState>>(gen, "uint32_t_ts_vector");

  py::class_<danet::ReflectionVar<uint32_t>>(gen, "uint32_t_var")
  .def_property_readonly("data", [](danet::ReflectionVar<uint32_t> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<uint32_t> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<uint32_t> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<uint8_t> bindings
  py::class_<danet::ReflectionVar<uint8_t>::TimeState>(gen, "uint8_t_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<uint8_t>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<uint8_t>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint8_t>::TimeState>>(gen, "uint8_t_ts_vector");

  py::class_<danet::ReflectionVar<uint8_t>>(gen, "uint8_t_var")
  .def_property_readonly("data", [](danet::ReflectionVar<uint8_t> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<uint8_t> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<uint8_t> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<uint32_t>> bindings
  py::class_<danet::ReflectionVar<std::vector<uint32_t>>::TimeState>(gen, "std_vector_uint32_t__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<uint32_t>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<uint32_t>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint32_t>>::TimeState>>(gen, "std_vector_uint32_t__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<uint32_t>>>(gen, "std_vector_uint32_t__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<uint32_t>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<uint32_t>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<uint32_t>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<uint16_t> bindings
  py::class_<danet::ReflectionVar<uint16_t>::TimeState>(gen, "uint16_t_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<uint16_t>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<uint16_t>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint16_t>::TimeState>>(gen, "uint16_t_ts_vector");

  py::class_<danet::ReflectionVar<uint16_t>>(gen, "uint16_t_var")
  .def_property_readonly("data", [](danet::ReflectionVar<uint16_t> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<uint16_t> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<uint16_t> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<ecs::EntityId> bindings
  py::class_<danet::ReflectionVar<ecs::EntityId>::TimeState>(gen, "ecs_EntityId_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<ecs::EntityId>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<ecs::EntityId>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<ecs::EntityId>::TimeState>>(gen, "ecs_EntityId_ts_vector");

  py::class_<danet::ReflectionVar<ecs::EntityId>>(gen, "ecs_EntityId_var")
  .def_property_readonly("data", [](danet::ReflectionVar<ecs::EntityId> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<ecs::EntityId> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<ecs::EntityId> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::Crew> bindings
  py::class_<danet::ReflectionVar<danet::Crew>::TimeState>(gen, "danet_Crew_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::Crew>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::Crew>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Crew>::TimeState>>(gen, "danet_Crew_ts_vector");

  py::class_<danet::ReflectionVar<danet::Crew>>(gen, "danet_Crew_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::Crew> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::Crew> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::Crew> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<danet::Crew>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::Crew>>::TimeState>(gen, "std_vector_danet_Crew__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::Crew>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<danet::Crew>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::Crew>>::TimeState>>(gen, "std_vector_danet_Crew__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::Crew>>>(gen, "std_vector_danet_Crew__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<danet::Crew>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<danet::Crew>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::Crew>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<bool> bindings
  py::class_<danet::ReflectionVar<bool>::TimeState>(gen, "bool_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<bool>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<bool>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<bool>::TimeState>>(gen, "bool_ts_vector");

  py::class_<danet::ReflectionVar<bool>>(gen, "bool_var")
  .def_property_readonly("data", [](danet::ReflectionVar<bool> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<bool> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<bool> &self){return &self.history();}, py::return_value_policy::reference_internal);


  include_types_1(gen);
}