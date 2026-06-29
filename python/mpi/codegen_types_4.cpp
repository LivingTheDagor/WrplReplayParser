#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_5(py::module &gen);
void include_types_4(py::module &gen) {
  // danet::ReflectionVar<danet::teamAvgEloRatings> bindings
  py::class_<danet::ReflectionVar<danet::teamAvgEloRatings>::SpaceHandler::TimeState>(gen, "danet_teamAvgEloRatings_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<danet::teamAvgEloRatings>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<danet::teamAvgEloRatings>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::teamAvgEloRatings>::SpaceHandler::TimeState>>(
    gen, "danet_teamAvgEloRatings_ts_vector");

  py::class_<danet::ReflectionVar<danet::teamAvgEloRatings>>(gen, "danet_teamAvgEloRatings_var")
    .def_readonly("data", &danet::ReflectionVar<danet::teamAvgEloRatings>::data)
    .def_property_readonly(
      "history", [](danet::ReflectionVar<danet::teamAvgEloRatings> &self) { return &self.get_history(); },
      py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::dummyForExitZonesSettings> bindings
  py::class_<danet::ReflectionVar<danet::dummyForExitZonesSettings>::SpaceHandler::TimeState>(
    gen, "danet_dummyForExitZonesSettings_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForExitZonesSettings>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<danet::dummyForExitZonesSettings>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForExitZonesSettings>::SpaceHandler::TimeState>>(
    gen, "danet_dummyForExitZonesSettings_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForExitZonesSettings>>(gen, "danet_dummyForExitZonesSettings_var")
  .def_readonly("data", &danet::ReflectionVar<danet::dummyForExitZonesSettings>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForExitZonesSettings> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::WeatherEffects> bindings
  py::class_<danet::ReflectionVar<danet::WeatherEffects>::SpaceHandler::TimeState>(gen, "danet_WeatherEffects_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::WeatherEffects>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::WeatherEffects>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeatherEffects>::SpaceHandler::TimeState>>(
    gen, "danet_WeatherEffects_ts_vector");

  py::class_<danet::ReflectionVar<danet::WeatherEffects>>(gen, "danet_WeatherEffects_var")
    .def_readonly("data", &danet::ReflectionVar<danet::WeatherEffects>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::WeatherEffects> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<Point2> bindings
  py::class_<danet::ReflectionVar<Point2>::SpaceHandler::TimeState>(gen, "Point2_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<Point2>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<Point2>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<Point2>::SpaceHandler::TimeState>>(gen, "Point2_ts_vector");

  py::class_<danet::ReflectionVar<Point2>>(gen, "Point2_var")
    .def_readonly("data", &danet::ReflectionVar<Point2>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<Point2> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::AreaFlagsEnum> bindings
  py::class_<danet::ReflectionVar<danet::AreaFlagsEnum>::SpaceHandler::TimeState>(gen, "danet_AreaFlagsEnum_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::AreaFlagsEnum>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::AreaFlagsEnum>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::AreaFlagsEnum>::SpaceHandler::TimeState>>(gen, "danet_AreaFlagsEnum_ts_vector");

  py::class_<danet::ReflectionVar<danet::AreaFlagsEnum>>(gen, "danet_AreaFlagsEnum_var")
  .def_readonly("data", &danet::ReflectionVar<danet::AreaFlagsEnum>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::AreaFlagsEnum> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<int8_t> bindings
  py::class_<danet::ReflectionVar<int8_t>::SpaceHandler::TimeState>(gen, "int8_t_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<int8_t>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<int8_t>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<int8_t>::SpaceHandler::TimeState>>(gen, "int8_t_ts_vector");

  py::class_<danet::ReflectionVar<int8_t>>(gen, "int8_t_var")
  .def_readonly("data", &danet::ReflectionVar<int8_t>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<int8_t> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<std::vector<danet::UnitId>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::UnitId>>::SpaceHandler::TimeState>(gen, "std_vector_danet_UnitId__ts")
    .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::UnitId>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<danet::UnitId>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::UnitId>>::SpaceHandler::TimeState>>(
    gen, "std_vector_danet_UnitId__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::UnitId>>>(gen, "std_vector_danet_UnitId__var")
  .def_readonly("data", &danet::ReflectionVar<std::vector<danet::UnitId>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::UnitId>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::UnitIdStruct> bindings
  py::class_<danet::ReflectionVar<danet::UnitIdStruct>::SpaceHandler::TimeState>(gen, "danet_UnitIdStruct_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::UnitIdStruct>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::UnitIdStruct>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::UnitIdStruct>::SpaceHandler::TimeState>>(gen, "danet_UnitIdStruct_ts_vector");

  py::class_<danet::ReflectionVar<danet::UnitIdStruct>>(gen, "danet_UnitIdStruct_var")
  .def_readonly("data", &danet::ReflectionVar<danet::UnitIdStruct>::data)
    .def_property_readonly(
      "history", [](danet::ReflectionVar<danet::UnitIdStruct> &self) { return &self.get_history(); },
      py::return_value_policy::reference_internal);


  include_types_5(gen);
}