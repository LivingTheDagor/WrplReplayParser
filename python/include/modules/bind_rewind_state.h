#pragma once
#include "Module.h"
#include "pybind11/numpy.h"
#include <cstring>

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

/// One field of every sample as a read-only numpy array. The history is kept in segments, not in one
/// buffer, so the values are copied (the history does not change once the replay is parsed).
template<typename H, typename TS, typename Field>
py::array channel_view(const H &hist, const TS *base, const Field *field, size_t count, const py::object &owner) {
  (void) owner;
  const size_t off = reinterpret_cast<const char *>(field) - reinterpret_cast<const char *>(base);
  py::array_t<Field> out({static_cast<py::ssize_t>(count)});
  Field *dst = out.mutable_data();
  hist.forEachSegment([&](const TS *p, uint32_t n) {
    for (uint32_t i = 0; i < n; i++)
      std::memcpy(dst++, reinterpret_cast<const char *>(p + i) + off, sizeof(Field));
  });
  py::detail::array_proxy(out.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
  return out;
}
