#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_3(py::module &gen);
void include_types_2(py::module &gen) {
  //danet::ReflectionVar<char> bindings
  py::class_<danet::ReflectionVar<char>::SpaceHandler::TimeState>(gen, "char_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<char>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<char>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<char>::SpaceHandler::TimeState>>(gen, "char_ts_vector");

  py::class_<danet::ReflectionVar<char>>(gen, "char_var")
  .def_readonly("data", &danet::ReflectionVar<char>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<char> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::WeatherEffect> bindings
  py::class_<danet::ReflectionVar<danet::WeatherEffect>::SpaceHandler::TimeState>(gen, "danet_WeatherEffect_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::WeatherEffect>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::WeatherEffect>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeatherEffect>::SpaceHandler::TimeState>>(
    gen, "danet_WeatherEffect_ts_vector");

  py::class_<danet::ReflectionVar<danet::WeatherEffect>>(gen, "danet_WeatherEffect_var")
  .def_readonly("data", &danet::ReflectionVar<danet::WeatherEffect>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::WeatherEffect> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<danet::WeatherEffect>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::SpaceHandler::TimeState>(gen, "std_vector_danet_WeatherEffect__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::WeatherEffect>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<danet::WeatherEffect>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::SpaceHandler::TimeState>>(
    gen, "std_vector_danet_WeatherEffect__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::WeatherEffect>>>(gen, "std_vector_danet_WeatherEffect__var")
  .def_readonly("data", &danet::ReflectionVar<std::vector<danet::WeatherEffect>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::WeatherEffect>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::UnitId> bindings
  py::class_<danet::ReflectionVar<danet::UnitId>::SpaceHandler::TimeState>(gen, "danet_UnitId_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::UnitId>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::UnitId>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::UnitId>::SpaceHandler::TimeState>>(
    gen, "danet_UnitId_ts_vector");

  py::class_<danet::ReflectionVar<danet::UnitId>>(gen, "danet_UnitId_var")
  .def_readonly("data", &danet::ReflectionVar<danet::UnitId>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::UnitId> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<uint64_t> bindings
  py::class_<danet::ReflectionVar<uint64_t>::SpaceHandler::TimeState>(gen, "uint64_t_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<uint64_t>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<uint64_t>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint64_t>::SpaceHandler::TimeState>>(gen, "uint64_t_ts_vector");

  py::class_<danet::ReflectionVar<uint64_t>>(gen, "uint64_t_var")
  .def_readonly("data", &danet::ReflectionVar<uint64_t>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<uint64_t> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::Uid> bindings
  py::class_<danet::ReflectionVar<danet::Uid>::SpaceHandler::TimeState>(gen, "danet_Uid_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::Uid>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::Uid>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Uid>::SpaceHandler::TimeState>>(gen,
                                                                                               "danet_Uid_ts_vector");

  py::class_<danet::ReflectionVar<danet::Uid>>(gen, "danet_Uid_var")
  .def_readonly("data", &danet::ReflectionVar<danet::Uid>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::Uid> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<DataBlock> bindings
  py::class_<danet::ReflectionVar<DataBlock>::SpaceHandler::TimeState>(gen, "DataBlock_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<DataBlock>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<DataBlock>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<DataBlock>::SpaceHandler::TimeState>>(gen,
                                                                                              "DataBlock_ts_vector");

  py::class_<danet::ReflectionVar<DataBlock>>(gen, "DataBlock_var")
  .def_readonly("data", &danet::ReflectionVar<DataBlock>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<DataBlock> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::Country> bindings
  py::class_<danet::ReflectionVar<danet::Country>::SpaceHandler::TimeState>(gen, "danet_Country_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<danet::Country>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<danet::Country>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Country>::SpaceHandler::TimeState>>(
    gen, "danet_Country_ts_vector");

  py::class_<danet::ReflectionVar<danet::Country>>(gen, "danet_Country_var")
    .def_readonly("data", &danet::ReflectionVar<danet::Country>::data)
    .def_property_readonly(
      "history", [](danet::ReflectionVar<danet::Country> &self) { return &self.get_history(); },
      py::return_value_policy::reference_internal);


  include_types_3(gen);
}