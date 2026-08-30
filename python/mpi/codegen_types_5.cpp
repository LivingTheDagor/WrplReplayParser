#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_6(py::module &gen);
void include_types_5(py::module &gen) {
  //danet::ReflectionVar<std::array<std::string,2>> bindings
  bind_time_state<std::array<std::string,2>>(gen, "std_array_std_string_2__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::array<std::string,2>>::TimeState>>(gen, "std_array_std_string_2__ts_vector");

  bind_reflection_var<std::array<std::string,2>>(gen, "std_array_std_string_2__var");
  //danet::ReflectionVar<danet::dummyForDeathInfo> bindings
  bind_time_state<danet::dummyForDeathInfo>(gen, "danet_dummyForDeathInfo_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForDeathInfo>::TimeState>>(gen, "danet_dummyForDeathInfo_ts_vector");

  bind_reflection_var<danet::dummyForDeathInfo>(gen, "danet_dummyForDeathInfo_var");
  //danet::ReflectionVar<danet::KillerStruct> bindings
  bind_time_state<danet::KillerStruct>(gen, "danet_KillerStruct_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::KillerStruct>::TimeState>>(gen, "danet_KillerStruct_ts_vector");

  bind_reflection_var<danet::KillerStruct>(gen, "danet_KillerStruct_var");
  //danet::ReflectionVar<danet::DamagedState> bindings
  bind_time_state<danet::DamagedState>(gen, "danet_DamagedState_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::DamagedState>::TimeState>>(gen, "danet_DamagedState_ts_vector");

  bind_reflection_var<danet::DamagedState>(gen, "danet_DamagedState_var");
  //danet::ReflectionVar<std::vector<danet::DamagedState>> bindings
  bind_time_state<std::vector<danet::DamagedState>>(gen, "std_vector_danet_DamagedState__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::DamagedState>>::TimeState>>(gen, "std_vector_danet_DamagedState__ts_vector");

  bind_reflection_var<std::vector<danet::DamagedState>>(gen, "std_vector_danet_DamagedState__var");
  //danet::ReflectionVar<danet::WeaponsMask> bindings
  bind_time_state<danet::WeaponsMask>(gen, "danet_WeaponsMask_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeaponsMask>::TimeState>>(gen, "danet_WeaponsMask_ts_vector");

  bind_reflection_var<danet::WeaponsMask>(gen, "danet_WeaponsMask_var");
  include_types_6(gen);
}