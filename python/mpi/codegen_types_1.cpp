#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_2(py::module &gen);
void include_types_1(py::module &gen) {
  //danet::ReflectionVar<danet::streak> bindings
  bind_time_state<danet::streak>(gen, "danet_streak_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::streak>::History>(gen, "danet_streak_ts_vector");

  bind_reflection_var<danet::streak>(gen, "danet_streak_var");
  //danet::ReflectionVar<std::vector<danet::streak>> bindings
  bind_time_state<std::vector<danet::streak>>(gen, "std_vector_danet_streak__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<danet::streak>>::History>(gen, "std_vector_danet_streak__ts_vector");

  bind_reflection_var<std::vector<danet::streak>>(gen, "std_vector_danet_streak__var");
  //danet::ReflectionVar<int> bindings
  bind_time_state<int>(gen, "int_ts");
  bind_readonly_vector<danet::ReflectionVar<int>::History>(gen, "int_ts_vector");

  bind_reflection_var<int>(gen, "int_var");
  //danet::ReflectionVar<danet::zigZagPair> bindings
  bind_time_state<danet::zigZagPair>(gen, "danet_zigZagPair_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::zigZagPair>::History>(gen, "danet_zigZagPair_ts_vector");

  bind_reflection_var<danet::zigZagPair>(gen, "danet_zigZagPair_var");
  //danet::ReflectionVar<std::vector<danet::zigZagPair>> bindings
  bind_time_state<std::vector<danet::zigZagPair>>(gen, "std_vector_danet_zigZagPair__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<danet::zigZagPair>>::History>(gen, "std_vector_danet_zigZagPair__ts_vector");

  bind_reflection_var<std::vector<danet::zigZagPair>>(gen, "std_vector_danet_zigZagPair__var");
  //danet::ReflectionVar<std::string> bindings
  bind_time_state<std::string>(gen, "std_string_ts");
  bind_readonly_vector<danet::ReflectionVar<std::string>::History>(gen, "std_string_ts_vector");

  bind_reflection_var<std::string>(gen, "std_string_var");
  //danet::ReflectionVar<char> bindings
  bind_time_state<char>(gen, "char_ts");
  bind_readonly_vector<danet::ReflectionVar<char>::History>(gen, "char_ts_vector");

  bind_reflection_var<char>(gen, "char_var");
  //danet::ReflectionVar<danet::WeatherEffect> bindings
  bind_time_state<danet::WeatherEffect>(gen, "danet_WeatherEffect_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::WeatherEffect>::History>(gen, "danet_WeatherEffect_ts_vector");

  bind_reflection_var<danet::WeatherEffect>(gen, "danet_WeatherEffect_var");
  include_types_2(gen);
}