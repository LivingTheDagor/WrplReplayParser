#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_4(py::module &gen);
void include_types_3(py::module &gen) {
  //danet::ReflectionVar<float> bindings
  bind_time_state<float>(gen, "float_ts");
  bind_readonly_vector<danet::ReflectionVar<float>::History>(gen, "float_ts_vector");

  bind_reflection_var<float>(gen, "float_var");
  //danet::ReflectionVar<std::vector<uint8_t>> bindings
  bind_time_state<std::vector<uint8_t>>(gen, "std_vector_uint8_t__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<uint8_t>>::History>(gen, "std_vector_uint8_t__ts_vector");

  bind_reflection_var<std::vector<uint8_t>>(gen, "std_vector_uint8_t__var");
  //danet::ReflectionVar<std::vector<uint16_t>> bindings
  bind_time_state<std::vector<uint16_t>>(gen, "std_vector_uint16_t__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<uint16_t>>::History>(gen, "std_vector_uint16_t__ts_vector");

  bind_reflection_var<std::vector<uint16_t>>(gen, "std_vector_uint16_t__var");
  //danet::ReflectionVar<danet::dummyForKillStreaksProgress> bindings
  bind_time_state<danet::dummyForKillStreaksProgress>(gen, "danet_dummyForKillStreaksProgress_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::dummyForKillStreaksProgress>::History>(gen, "danet_dummyForKillStreaksProgress_ts_vector");

  bind_reflection_var<danet::dummyForKillStreaksProgress>(gen, "danet_dummyForKillStreaksProgress_var");
  //danet::ReflectionVar<danet::RoundScore> bindings
  bind_time_state<danet::RoundScore>(gen, "danet_RoundScore_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::RoundScore>::History>(gen, "danet_RoundScore_ts_vector");

  bind_reflection_var<danet::RoundScore>(gen, "danet_RoundScore_var");
  //danet::ReflectionVar<danet::dummyForPlayerStat> bindings
  bind_time_state<danet::dummyForPlayerStat>(gen, "danet_dummyForPlayerStat_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::dummyForPlayerStat>::History>(gen, "danet_dummyForPlayerStat_ts_vector");

  bind_reflection_var<danet::dummyForPlayerStat>(gen, "danet_dummyForPlayerStat_var");
  //danet::ReflectionVar<danet::dummyForFootballStat> bindings
  bind_time_state<danet::dummyForFootballStat>(gen, "danet_dummyForFootballStat_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::dummyForFootballStat>::History>(gen, "danet_dummyForFootballStat_ts_vector");

  bind_reflection_var<danet::dummyForFootballStat>(gen, "danet_dummyForFootballStat_var");
  //danet::ReflectionVar<danet::Uid> bindings
  bind_time_state<danet::Uid>(gen, "danet_Uid_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::Uid>::History>(gen, "danet_Uid_ts_vector");

  bind_reflection_var<danet::Uid>(gen, "danet_Uid_var");
  include_types_4(gen);
}