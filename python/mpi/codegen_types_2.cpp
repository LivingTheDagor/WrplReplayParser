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
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::WeatherEffect>>::TimeState>>(gen, "std_vector_danet_WeatherEffect__ts_vector");

  bind_reflection_var<std::vector<danet::WeatherEffect>>(gen, "std_vector_danet_WeatherEffect__var");
  //danet::ReflectionVar<danet::UnitId> bindings
  bind_time_state<danet::UnitId>(gen, "danet_UnitId_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::UnitId>::TimeState>>(gen, "danet_UnitId_ts_vector");

  bind_reflection_var<danet::UnitId>(gen, "danet_UnitId_var");
  //danet::ReflectionVar<uint64_t> bindings
  bind_time_state<uint64_t>(gen, "uint64_t_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint64_t>::TimeState>>(gen, "uint64_t_ts_vector");

  bind_reflection_var<uint64_t>(gen, "uint64_t_var");
  //danet::ReflectionVar<danet::Uid> bindings
  bind_time_state<danet::Uid>(gen, "danet_Uid_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Uid>::TimeState>>(gen, "danet_Uid_ts_vector");

  bind_reflection_var<danet::Uid>(gen, "danet_Uid_var");
  //danet::ReflectionVar<DataBlock> bindings
  bind_time_state<DataBlock>(gen, "DataBlock_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<DataBlock>::TimeState>>(gen, "DataBlock_ts_vector");

  bind_reflection_var<DataBlock>(gen, "DataBlock_var");
  //danet::ReflectionVar<danet::Country> bindings
  bind_time_state<danet::Country>(gen, "danet_Country_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Country>::TimeState>>(gen, "danet_Country_ts_vector");

  bind_reflection_var<danet::Country>(gen, "danet_Country_var");
  //danet::ReflectionVar<std::array<ecs::EntityId,20>> bindings
  bind_time_state<std::array<ecs::EntityId,20>>(gen, "std_array_ecs_EntityId_20__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::array<ecs::EntityId,20>>::TimeState>>(gen, "std_array_ecs_EntityId_20__ts_vector");

  bind_reflection_var<std::array<ecs::EntityId,20>>(gen, "std_array_ecs_EntityId_20__var");
  //danet::ReflectionVar<danet::CrewUnitsList> bindings
  bind_time_state<danet::CrewUnitsList>(gen, "danet_CrewUnitsList_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::CrewUnitsList>::TimeState>>(gen, "danet_CrewUnitsList_ts_vector");

  bind_reflection_var<danet::CrewUnitsList>(gen, "danet_CrewUnitsList_var");
  include_types_3(gen);
}