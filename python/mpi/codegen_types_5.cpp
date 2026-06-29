#include "modules/mpi/mpi.h"
#include "modules/mpi/codegen_types.h"
#include "modules/bind_readonly_vector.h"
#include "mpi/reflection.h"
#include "mpi/types.h"
#include "modules/mpi/bind_array.h"
#include "pybind11/stl_bind.h"
void include_types_6(py::module &gen);
void include_types_5(py::module &gen) {
  // danet::ReflectionVar<std::vector<danet::UnitIdStruct>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::SpaceHandler::TimeState>(
    gen, "std_vector_danet_UnitIdStruct__ts")
    .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::SpaceHandler::TimeState>>(
    gen, "std_vector_danet_UnitIdStruct__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::UnitIdStruct>>>(gen, "std_vector_danet_UnitIdStruct__var")
    .def_readonly("data", &danet::ReflectionVar<std::vector<danet::UnitIdStruct>>::data)
    .def_property_readonly(
      "history", [](danet::ReflectionVar<std::vector<danet::UnitIdStruct>> &self) { return &self.get_history(); },
      py::return_value_policy::reference_internal);


  // danet::ReflectionVar<std::array<std::string,2>> bindings
  py::class_<danet::ReflectionVar<std::array<std::string, 2>>::SpaceHandler::TimeState>(gen,
                                                                                        "std_array_std_string_2__ts")
    .def_readonly("time_ms", &danet::ReflectionVar<std::array<std::string, 2>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::array<std::string, 2>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::array<std::string, 2>>::SpaceHandler::TimeState>>(
    gen, "std_array_std_string_2__ts_vector");

  py::class_<danet::ReflectionVar<std::array<std::string,2>>>(gen, "std_array_std_string_2__var")
  .def_readonly("data", &danet::ReflectionVar<std::array<std::string,2>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::array<std::string,2>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::dummyForDeathInfo> bindings
  py::class_<danet::ReflectionVar<danet::dummyForDeathInfo>::SpaceHandler::TimeState>(gen, "danet_dummyForDeathInfo_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::dummyForDeathInfo>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::dummyForDeathInfo>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::dummyForDeathInfo>::SpaceHandler::TimeState>>(
    gen, "danet_dummyForDeathInfo_ts_vector");

  py::class_<danet::ReflectionVar<danet::dummyForDeathInfo>>(gen, "danet_dummyForDeathInfo_var")
    .def_readonly("data", &danet::ReflectionVar<danet::dummyForDeathInfo>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::dummyForDeathInfo> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::KillerStruct> bindings
  py::class_<danet::ReflectionVar<danet::KillerStruct>::SpaceHandler::TimeState>(gen, "danet_KillerStruct_ts")
    .def_readonly("time_ms", &danet::ReflectionVar<danet::KillerStruct>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<danet::KillerStruct>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::KillerStruct>::SpaceHandler::TimeState>>(
    gen, "danet_KillerStruct_ts_vector");

  py::class_<danet::ReflectionVar<danet::KillerStruct>>(gen, "danet_KillerStruct_var")
  .def_readonly("data", &danet::ReflectionVar<danet::KillerStruct>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::KillerStruct> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::DamagedState> bindings
  py::class_<danet::ReflectionVar<danet::DamagedState>::SpaceHandler::TimeState>(gen, "danet_DamagedState_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::DamagedState>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::DamagedState>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::DamagedState>::SpaceHandler::TimeState>>(
    gen, "danet_DamagedState_ts_vector");

  py::class_<danet::ReflectionVar<danet::DamagedState>>(gen, "danet_DamagedState_var")
    .def_readonly("data", &danet::ReflectionVar<danet::DamagedState>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::DamagedState> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<std::vector<danet::DamagedState>> bindings
  py::class_<danet::ReflectionVar<std::vector<danet::DamagedState>>::SpaceHandler::TimeState>(
    gen, "std_vector_danet_DamagedState__ts")
    .def_readonly("time_ms", &danet::ReflectionVar<std::vector<danet::DamagedState>>::SpaceHandler::TimeState::time_ms)
    .def_readonly("value", &danet::ReflectionVar<std::vector<danet::DamagedState>>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<std::vector<danet::DamagedState>>::SpaceHandler::TimeState>>(
    gen, "std_vector_danet_DamagedState__ts_vector");

  py::class_<danet::ReflectionVar<std::vector<danet::DamagedState>>>(gen, "std_vector_danet_DamagedState__var")
  .def_readonly("data", &danet::ReflectionVar<std::vector<danet::DamagedState>>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<std::vector<danet::DamagedState>> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  // danet::ReflectionVar<danet::WeaponsMask> bindings
  py::class_<danet::ReflectionVar<danet::WeaponsMask>::SpaceHandler::TimeState>(gen, "danet_WeaponsMask_ts")
  .def_readonly("time_ms", &danet::ReflectionVar<danet::WeaponsMask>::SpaceHandler::TimeState::time_ms)
  .def_readonly("value", &danet::ReflectionVar<danet::WeaponsMask>::SpaceHandler::TimeState::data);

  bind_readonly_vector<dag::Vector<danet::ReflectionVar<danet::WeaponsMask>::SpaceHandler::TimeState>>(
    gen, "danet_WeaponsMask_ts_vector");

  py::class_<danet::ReflectionVar<danet::WeaponsMask>>(gen, "danet_WeaponsMask_var")
    .def_readonly("data", &danet::ReflectionVar<danet::WeaponsMask>::data)
  .def_property_readonly("history", [](danet::ReflectionVar<danet::WeaponsMask> &self){return &self.get_history();}, py::return_value_policy::reference_internal);


  include_types_6(gen);
}