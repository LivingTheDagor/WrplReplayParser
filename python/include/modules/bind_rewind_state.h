#pragma once
#include "Module.h"
#include "pybind11/numpy.h"

template<typename State>
py::class_<State> bind_rewind_state(py::module_ &m, const char *name) {
  return py::class_<State>(m, name)
    .def_property_readonly(
      "data",
      [](State &self) {
        if (!self.hasData())
          throw py::index_error("history is empty");
        return *self.curr();
      },
      py::return_value_policy::reference_internal,
      "Latest value. Raises IndexError when the history is empty, which is possible here:\n"
      "unlike the *_var containers these histories have no synthetic zero sample.")
    .def_property_readonly(
      "time_ms",
      [](State &self) {
        if (!self.hasData())
          throw py::index_error("history is empty");
        return self.currState()->time_ms;
      },
      "Timestamp of the latest value, in ms. Raises IndexError when the history is empty.")
    .def_property_readonly(
      "history", [](State &self) { return &self.history(); }, py::return_value_policy::reference_internal,
      "All samples as a list-like of TimeState. Not iterable itself and negative indexing\n"
      "is not supported, wrap it in list() for that. Prefer channels() for bulk access.");
}

template<typename TS, typename Field>
py::array channel_view(const TS *base, const Field *field, size_t count, const py::object &owner) {
  (void) base;
  py::array_t<Field> view({count}, {sizeof(TS)}, field, owner);
  py::detail::array_proxy(view.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
  return view;
}
