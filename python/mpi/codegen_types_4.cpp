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
  //danet::ReflectionVar<Point3> bindings
  bind_time_state<Point3>(gen, "Point3_ts");
  bind_readonly_vector<danet::ReflectionVar<Point3>::History>(gen, "Point3_ts_vector");

  bind_reflection_var<Point3>(gen, "Point3_var");
  //danet::ReflectionVar<danet::dummyForExitZonesSettings> bindings
  bind_time_state<danet::dummyForExitZonesSettings>(gen, "danet_dummyForExitZonesSettings_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::dummyForExitZonesSettings>::History>(gen, "danet_dummyForExitZonesSettings_ts_vector");

  bind_reflection_var<danet::dummyForExitZonesSettings>(gen, "danet_dummyForExitZonesSettings_var");
  //danet::ReflectionVar<danet::WeatherEffects> bindings
  bind_time_state<danet::WeatherEffects>(gen, "danet_WeatherEffects_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::WeatherEffects>::History>(gen, "danet_WeatherEffects_ts_vector");

  bind_reflection_var<danet::WeatherEffects>(gen, "danet_WeatherEffects_var");
  //danet::ReflectionVar<Point2> bindings
  bind_time_state<Point2>(gen, "Point2_ts");
  bind_readonly_vector<danet::ReflectionVar<Point2>::History>(gen, "Point2_ts_vector");

  bind_reflection_var<Point2>(gen, "Point2_var");
  //danet::ReflectionVar<danet::AreaFlagsEnum> bindings
  bind_time_state<danet::AreaFlagsEnum>(gen, "danet_AreaFlagsEnum_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::AreaFlagsEnum>::History>(gen, "danet_AreaFlagsEnum_ts_vector");

  bind_reflection_var<danet::AreaFlagsEnum>(gen, "danet_AreaFlagsEnum_var");
  //danet::ReflectionVar<int8_t> bindings
  bind_time_state<int8_t>(gen, "int8_t_ts");
  bind_readonly_vector<danet::ReflectionVar<int8_t>::History>(gen, "int8_t_ts_vector");

  bind_reflection_var<int8_t>(gen, "int8_t_var");
  //danet::ReflectionVar<std::vector<danet::UnitId>> bindings
  bind_time_state<std::vector<danet::UnitId>>(gen, "std_vector_danet_UnitId__ts");
  bind_readonly_vector<danet::ReflectionVar<std::vector<danet::UnitId>>::History>(gen, "std_vector_danet_UnitId__ts_vector");

  bind_reflection_var<std::vector<danet::UnitId>>(gen, "std_vector_danet_UnitId__var");
  //danet::ReflectionVar<danet::UnitIdStruct> bindings
  bind_time_state<danet::UnitIdStruct>(gen, "danet_UnitIdStruct_ts");
  bind_readonly_vector<danet::ReflectionVar<danet::UnitIdStruct>::History>(gen, "danet_UnitIdStruct_ts_vector");

  bind_reflection_var<danet::UnitIdStruct>(gen, "danet_UnitIdStruct_var");
  include_types_5(gen);
}