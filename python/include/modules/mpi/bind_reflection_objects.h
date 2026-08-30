#pragma once
#include "modules/DataBlock/DataBlock.h"

template<typename T>
py::class_<typename danet::ReflectionVar<T>::TimeState> bind_time_state(py::module_ &m, const std::string &name) {
  using TS = typename danet::ReflectionVar<T>::TimeState;
  return py::class_<TS>(m, name.c_str()).def_readonly("time_ms", &TS::time_ms).def_readonly("value", &TS::data);
}

template<>
inline py::class_<danet::ReflectionVar<DataBlock>::TimeState> bind_time_state<DataBlock>(py::module_ &m,
                                                                                         const std::string &name) {
  using TS = danet::ReflectionVar<DataBlock>::TimeState;
  return py::class_<TS>(m, name.c_str())
    .def_readonly("time_ms", &TS::time_ms)
    .def_property_readonly("value", [](const TS &self) { return DataBlockRO(&self.data); });
}


template<typename T>
py::class_<danet::ReflectionVar<T>> bind_reflection_var(py::module_ &m, const std::string &name) {
  using RV = danet::ReflectionVar<T>;
  return py::class_<RV>(m, name.c_str())
    .def_property_readonly(
      "data", [](RV &self) { return self.curr(); }, py::return_value_policy::reference_internal)
    .def_property_readonly("time_ms", [](RV &self) { return self.currState()->time_ms; })
    .def_property_readonly(
      "history", [](RV &self) { return &self.history(); }, py::return_value_policy::reference_internal);
}

template<>
inline py::class_<danet::ReflectionVar<DataBlock>> bind_reflection_var<DataBlock>(py::module_ &m,
                                                                                  const std::string &name) {
  using RV = danet::ReflectionVar<DataBlock>;
  return py::class_<RV>(m, name.c_str())
    .def_property_readonly(
      "data", [](RV &self) { return DataBlockRO(self.curr()); }, py::return_value_policy::reference_internal)
    .def_property_readonly("time_ms", [](RV &self) { return self.currState()->time_ms; })
    .def_property_readonly(
      "history", [](RV &self) { return &self.history(); }, py::return_value_policy::reference_internal);
}
