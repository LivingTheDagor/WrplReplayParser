#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "modules/mpi/bind_reflection_objects.h"
#include "pybind11/stl_bind.h"
void include_types_5(py::module &gen);
void include_types_4(py::module &gen) {
  //danet::ReflectionVar<danet::dummyForExitZonesSettings> bindings
  bind_time_state<danet::dummyForExitZonesSettings>(gen, "danet_dummyForExitZonesSettings_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForExitZonesSettings>::TimeState>>(gen, "danet_dummyForExitZonesSettings_ts_vector");

  bind_reflection_var<danet::dummyForExitZonesSettings>(gen, "danet_dummyForExitZonesSettings_var");
  //danet::ReflectionVar<danet::WeatherEffects> bindings
  bind_time_state<danet::WeatherEffects>(gen, "danet_WeatherEffects_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeatherEffects>::TimeState>>(gen, "danet_WeatherEffects_ts_vector");

  bind_reflection_var<danet::WeatherEffects>(gen, "danet_WeatherEffects_var");
  //danet::ReflectionVar<Point2> bindings
  bind_time_state<Point2>(gen, "Point2_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<Point2>::TimeState>>(gen, "Point2_ts_vector");

  bind_reflection_var<Point2>(gen, "Point2_var");
  //danet::ReflectionVar<danet::AreaFlagsEnum> bindings
  bind_time_state<danet::AreaFlagsEnum>(gen, "danet_AreaFlagsEnum_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::AreaFlagsEnum>::TimeState>>(gen, "danet_AreaFlagsEnum_ts_vector");

  bind_reflection_var<danet::AreaFlagsEnum>(gen, "danet_AreaFlagsEnum_var");
  //danet::ReflectionVar<int8_t> bindings
  bind_time_state<int8_t>(gen, "int8_t_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<int8_t>::TimeState>>(gen, "int8_t_ts_vector");

  bind_reflection_var<int8_t>(gen, "int8_t_var");
  //danet::ReflectionVar<std::vector<danet::UnitId>> bindings
  bind_time_state<std::vector<danet::UnitId>>(gen, "std_vector_danet_UnitId__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::UnitId>>::TimeState>>(gen, "std_vector_danet_UnitId__ts_vector");

  bind_reflection_var<std::vector<danet::UnitId>>(gen, "std_vector_danet_UnitId__var");
  //danet::ReflectionVar<danet::UnitIdStruct> bindings
  bind_time_state<danet::UnitIdStruct>(gen, "danet_UnitIdStruct_ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::UnitIdStruct>::TimeState>>(gen, "danet_UnitIdStruct_ts_vector");

  bind_reflection_var<danet::UnitIdStruct>(gen, "danet_UnitIdStruct_var");
  //danet::ReflectionVar<std::vector<danet::UnitIdStruct>> bindings
  bind_time_state<std::vector<danet::UnitIdStruct>>(gen, "std_vector_danet_UnitIdStruct__ts");
  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::TimeState>>(gen, "std_vector_danet_UnitIdStruct__ts_vector");

  bind_reflection_var<std::vector<danet::UnitIdStruct>>(gen, "std_vector_danet_UnitIdStruct__var");
  include_types_5(gen);
}