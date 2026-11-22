#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_3(py::module &gen);
void include_types_2(py::module &gen) {
  //danet::ReflectionVar<std::vector<danet::WeatherEffect>> bindings
  bind_time_state<std::vector<danet::WeatherEffect>>(gen, "std_vector_danet_WeatherEffect__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::History>(gen, "std_vector_danet_WeatherEffect__ts_vector");

  bind_reflection_var<std::vector<danet::WeatherEffect>>(gen, "std_vector_danet_WeatherEffect__var");
  //danet::ReflectionVar<danet::UnitId> bindings
  bind_time_state<danet::UnitId>(gen, "danet_UnitId_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::UnitId>::History>(gen, "danet_UnitId_ts_vector");

  bind_reflection_var<danet::UnitId>(gen, "danet_UnitId_var");
  //danet::ReflectionVar<int64_t> bindings
  bind_time_state<int64_t>(gen, "int64_t_ts");
  bind_readonly_vector<danet::ReflectionVar<int64_t>::History>(gen, "int64_t_ts_vector");

  bind_reflection_var<int64_t>(gen, "int64_t_var");
  //danet::ReflectionVar<std::vector<ecs::EntityId>> bindings
  bind_time_state<std::vector<ecs::EntityId>>(gen, "std_vector_ecs_EntityId__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<ecs::EntityId>>::History>(gen, "std_vector_ecs_EntityId__ts_vector");

  bind_reflection_var<std::vector<ecs::EntityId>>(gen, "std_vector_ecs_EntityId__var");
  //danet::ReflectionVar<DataBlock> bindings
  bind_time_state<DataBlock>(gen, "DataBlock_ts");
  bind_readonly_vector<danet::ReflectionVar<DataBlock>::History>(gen, "DataBlock_ts_vector");

  bind_reflection_var<DataBlock>(gen, "DataBlock_var");
  //danet::ReflectionVar<danet::Country> bindings
  bind_time_state<danet::Country>(gen, "danet_Country_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::Country>::History>(gen, "danet_Country_ts_vector");

  bind_reflection_var<danet::Country>(gen, "danet_Country_var");
  //danet::ReflectionVar<danet::dummyForSupportPlanes> bindings
  bind_time_state<danet::dummyForSupportPlanes>(gen, "danet_dummyForSupportPlanes_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::dummyForSupportPlanes>::History>(gen, "danet_dummyForSupportPlanes_ts_vector");

  bind_reflection_var<danet::dummyForSupportPlanes>(gen, "danet_dummyForSupportPlanes_var");
  //danet::ReflectionVar<danet::CrewUnitsList> bindings
  bind_time_state<danet::CrewUnitsList>(gen, "danet_CrewUnitsList_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::CrewUnitsList>::History>(gen, "danet_CrewUnitsList_ts_vector");

  bind_reflection_var<danet::CrewUnitsList>(gen, "danet_CrewUnitsList_var");
  include_types_3(gen);
}