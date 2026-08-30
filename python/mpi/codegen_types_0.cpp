#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_1(py::module &gen);
void include_types_0(py::module &gen) {
  //danet::ReflectionVar<uint32_t> bindings
  bind_time_state<uint32_t>(gen, "uint32_t_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint32_t>::TimeState>>(gen, "uint32_t_ts_vector");

  bind_reflection_var<uint32_t>(gen, "uint32_t_var");
  //danet::ReflectionVar<uint8_t> bindings
  bind_time_state<uint8_t>(gen, "uint8_t_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint8_t>::TimeState>>(gen, "uint8_t_ts_vector");

  bind_reflection_var<uint8_t>(gen, "uint8_t_var");
  //danet::ReflectionVar<std::vector<uint32_t>> bindings
  bind_time_state<std::vector<uint32_t>>(gen, "std_vector_uint32_t__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<uint32_t>>::TimeState>>(gen, "std_vector_uint32_t__ts_vector");

  bind_reflection_var<std::vector<uint32_t>>(gen, "std_vector_uint32_t__var");
  //danet::ReflectionVar<uint16_t> bindings
  bind_time_state<uint16_t>(gen, "uint16_t_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<uint16_t>::TimeState>>(gen, "uint16_t_ts_vector");

  bind_reflection_var<uint16_t>(gen, "uint16_t_var");
  //danet::ReflectionVar<ecs::EntityId> bindings
  bind_time_state<ecs::EntityId>(gen, "ecs_EntityId_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<ecs::EntityId>::TimeState>>(gen, "ecs_EntityId_ts_vector");

  bind_reflection_var<ecs::EntityId>(gen, "ecs_EntityId_var");
  //danet::ReflectionVar<danet::Crew> bindings
  bind_time_state<danet::Crew>(gen, "danet_Crew_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::Crew>::TimeState>>(gen, "danet_Crew_ts_vector");

  bind_reflection_var<danet::Crew>(gen, "danet_Crew_var");
  //danet::ReflectionVar<std::vector<danet::Crew>> bindings
  bind_time_state<std::vector<danet::Crew>>(gen, "std_vector_danet_Crew__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::Crew>>::TimeState>>(gen, "std_vector_danet_Crew__ts_vector");

  bind_reflection_var<std::vector<danet::Crew>>(gen, "std_vector_danet_Crew__var");
  //danet::ReflectionVar<bool> bindings
  bind_time_state<bool>(gen, "bool_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<bool>::TimeState>>(gen, "bool_ts_vector");

  bind_reflection_var<bool>(gen, "bool_var");
  include_types_1(gen);
}