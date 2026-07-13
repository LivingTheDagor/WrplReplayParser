#include "Module.h"
#include <array>
#include <stdexcept>
#include <sstream>

template <typename T>
concept has_to_string = requires(const T t, int i) {
    { t.toString(i) } -> std::convertible_to<std::string>;
};
template <typename T, std::size_t N>
py::class_<std::array<T, N>> bind_array(py::module_& m, const std::string& name) {
  using Array = std::array<T, N>;

  auto cl = py::class_<Array>(m, name.c_str(), py::buffer_protocol())
      .def(py::init<>())

          // __len__
      .def("__len__", [](const Array& a) { return N; })

          // __getitem__ with bounds checking
      .def("__getitem__", [](const Array& a, std::size_t i) -> T {
        if (i >= N) throw py::index_error("index out of range");
        return a[i];
      })

          // __iter__
      .def("__iter__", [](const Array& a) {
        return py::make_iterator(a.begin(), a.end());
      }, py::keep_alive<0, 1>())

          // __repr__
      .def("__repr__", [name](const Array& a) {
        std::ostringstream os;
        os << name << "[";
        for (std::size_t i = 0; i < N; ++i) {
          if (i > 0) os << ", ";


          if constexpr (has_to_string<T>) {
              os << a[i].toString(0);
          } else {
              os << a[i];
          }
        }
        os << "]";
        return os.str();
      })

          // __eq__
      .def("__eq__", [](const Array& a, const Array& b) { return a == b; })

          // Expose size as a property
      .def_property_readonly_static("size", [](py::object) { return N; });

  // Buffer protocol support (for numeric types) — allows numpy interop
  if constexpr (std::is_arithmetic_v<T>) {
    cl.def_buffer([](Array& a) {
      return py::buffer_info(
          a.data(),
          sizeof(T),
          py::format_descriptor<T>::format(),
          1,                   // ndim
          { N },               // shape
          { sizeof(T) }        // strides
      );
    });
  }

  return cl;
}