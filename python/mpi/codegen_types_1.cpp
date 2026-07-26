#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_2(py::module &gen);
void include_types_1(py::module &gen) {
  //danet::ReflectionVar<danet::streak> bindings
  py::class_<danet::ReflectionVar<danet::streak>::TimeState>(gen, "danet_streak_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::streak>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::streak>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::streak>::TimeState>>(gen, "danet_streak_ts_vector");

  py::class_<danet::ReflectionVar<danet::streak>>(gen, "danet_streak_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::streak> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::streak> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::streak> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<danet::streak>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::streak>>::TimeState>(gen, "std_vector_danet_streak__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::streak>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<danet::streak>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::streak>>::TimeState>>(gen, "std_vector_danet_streak__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::streak>>>(gen, "std_vector_danet_streak__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<danet::streak>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<danet::streak>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::streak>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<int> bindings
  py::class_<danet::ReflectionVar<int>::TimeState>(gen, "int_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<int>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<int>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<int>::TimeState>>(gen, "int_ts_vector");

  py::class_<danet::ReflectionVar<int>>(gen, "int_var")
  .def_property_readonly("data", [](danet::ReflectionVar<int> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<int> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<int> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::zigZagPair> bindings
  py::class_<danet::ReflectionVar<danet::zigZagPair>::TimeState>(gen, "danet_zigZagPair_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::zigZagPair>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::zigZagPair>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::zigZagPair>::TimeState>>(gen, "danet_zigZagPair_ts_vector");

  py::class_<danet::ReflectionVar<danet::zigZagPair>>(gen, "danet_zigZagPair_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::zigZagPair> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::zigZagPair> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::zigZagPair> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::vector<danet::zigZagPair>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::zigZagPair>>::TimeState>(gen, "std_vector_danet_zigZagPair__ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::zigZagPair>>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::vector<danet::zigZagPair>>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::zigZagPair>>::TimeState>>(gen, "std_vector_danet_zigZagPair__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::zigZagPair>>>(gen, "std_vector_danet_zigZagPair__var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::vector<danet::zigZagPair>> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::vector<danet::zigZagPair>> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::zigZagPair>> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<std::string> bindings
  py::class_<danet::ReflectionVar<std::string>::TimeState>(gen, "std_string_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<std::string>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<std::string>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::string>::TimeState>>(gen, "std_string_ts_vector");

  py::class_<danet::ReflectionVar<std::string>>(gen, "std_string_var")
  .def_property_readonly("data", [](danet::ReflectionVar<std::string> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<std::string> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<std::string> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<char> bindings
  py::class_<danet::ReflectionVar<char>::TimeState>(gen, "char_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<char>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<char>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<char>::TimeState>>(gen, "char_ts_vector");

  py::class_<danet::ReflectionVar<char>>(gen, "char_var")
  .def_property_readonly("data", [](danet::ReflectionVar<char> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<char> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<char> &self){return &self.history();}, py::return_value_policy::reference_internal);


  //danet::ReflectionVar<danet::WeatherEffect> bindings
  py::class_<danet::ReflectionVar<danet::WeatherEffect>::TimeState>(gen, "danet_WeatherEffect_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::WeatherEffect>::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::WeatherEffect>::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeatherEffect>::TimeState>>(gen, "danet_WeatherEffect_ts_vector");

  py::class_<danet::ReflectionVar<danet::WeatherEffect>>(gen, "danet_WeatherEffect_var")
  .def_property_readonly("data", [](danet::ReflectionVar<danet::WeatherEffect> &self){return self.curr();}, py::return_value_policy::reference_internal)
  .def_property_readonly("time_ms", [](danet::ReflectionVar<danet::WeatherEffect> &self){return self.currState()->time_ms;})
  .def_property_readonly("history", [](danet::ReflectionVar<danet::WeatherEffect> &self){return &self.history();}, py::return_value_policy::reference_internal);


  include_types_2(gen);
}