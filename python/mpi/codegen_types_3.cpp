#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_4(py::module &gen);
void include_types_3(py::module &gen) {
  //danet::ReflectionVar<float> bindings
  py::class_<danet::ReflectionVar<float>::TimeState>(gen, "float_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<float>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<float>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<float>::TimeState>>(gen, "float_ts_vector");

  py::class_<danet::ReflectionVar<float>>(gen, "float_var")
  .def_property_readonly("data", [](danet::ReflectionVar<float> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<float> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<float> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<uint8_t>> bindings
  py::class_<danet::ReflectionVar<std::vector<uint8_t>>::TimeState>(gen, "std_vector_uint8_t__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<uint8_t>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<uint8_t>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint8_t>>::TimeState>>(gen, "std_vector_uint8_t__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<uint8_t>>>(gen, "std_vector_uint8_t__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<uint8_t>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<uint8_t>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<uint8_t>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<uint16_t>> bindings
  py::class_<danet::ReflectionVar<std::vector<uint16_t>>::TimeState>(gen, "std_vector_uint16_t__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<uint16_t>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<uint16_t>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint16_t>>::TimeState>>(gen, "std_vector_uint16_t__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<uint16_t>>>(gen, "std_vector_uint16_t__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<uint16_t>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<uint16_t>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<uint16_t>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::dummyForKillStreaksProgress> bindings
  py::class_<danet::ReflectionVar<danet::dummyForKillStreaksProgress>::TimeState>(gen, "danet_dummyForKillStreaksProgress_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForKillStreaksProgress>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForKillStreaksProgress>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForKillStreaksProgress>::TimeState>>(gen, "danet_dummyForKillStreaksProgress_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForKillStreaksProgress>>(gen, "danet_dummyForKillStreaksProgress_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::dummyForKillStreaksProgress> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::dummyForKillStreaksProgress> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForKillStreaksProgress> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::RoundScore> bindings
  py::class_<danet::ReflectionVar<danet::RoundScore>::TimeState>(gen, "danet_RoundScore_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::RoundScore>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::RoundScore>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::RoundScore>::TimeState>>(gen, "danet_RoundScore_ts_vector");

  py::class_<danet::ReflectionVar<danet::RoundScore>>(gen, "danet_RoundScore_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::RoundScore> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::RoundScore> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::RoundScore> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::dummyForPlayerStat> bindings
  py::class_<danet::ReflectionVar<danet::dummyForPlayerStat>::TimeState>(gen, "danet_dummyForPlayerStat_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForPlayerStat>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForPlayerStat>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForPlayerStat>::TimeState>>(gen, "danet_dummyForPlayerStat_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForPlayerStat>>(gen, "danet_dummyForPlayerStat_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::dummyForPlayerStat> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::dummyForPlayerStat> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForPlayerStat> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::dummyForFootballStat> bindings
  py::class_<danet::ReflectionVar<danet::dummyForFootballStat>::TimeState>(gen, "danet_dummyForFootballStat_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForFootballStat>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForFootballStat>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForFootballStat>::TimeState>>(gen, "danet_dummyForFootballStat_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForFootballStat>>(gen, "danet_dummyForFootballStat_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::dummyForFootballStat> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::dummyForFootballStat> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForFootballStat> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<Point3> bindings
  py::class_<danet::ReflectionVar<Point3>::TimeState>(gen, "Point3_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<Point3>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<Point3>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<Point3>::TimeState>>(gen, "Point3_ts_vector");

  py::class_<danet::ReflectionVar<Point3>>(gen, "Point3_var")
  .def_property_readonly("data", [](danet::ReflectionVar<Point3> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<Point3> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<Point3> &self){return &self.history();}, py::return_value_policy::reference_internal);


  include_types_4(gen);
}