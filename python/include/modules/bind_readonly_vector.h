
#pragma once
#include "Module.h"
#include "sstream"

template<typename Vec, typename CName = Vec>
void bind_readonly_vector(py::module_ &m, const char *name) {
  py::class_<Vec>(m, name)
      .def("__len__", [](const Vec &v) { return v.size(); })
    .def("__getitem__",
         [](const Vec &v, typename Vec::size_type i) {
           if (i >= v.size()) throw py::index_error();
           return v[i];
      })
      .def(
          "__iter__", [](const Vec &v) -> py::iterator { return py::make_iterator(v.begin(), v.end());
      }, py::keep_alive<0, 1>()) // Keep vector alive while iterator exists
      .def("__contains__", [](const Vec &v, const typename Vec::value_type &val) {
        return std::find(v.begin(), v.end(), val) != v.end();
      })
      .def("__repr__", [name](const Vec &v) {
        std::ostringstream oss;
        oss << name << "(";
        oss << "[";
        for (typename Vec::size_type i = 0; i < v.size(); ++i) {
          if (i) oss << ", ";
          oss << py::repr(py::cast(v[i]));
        }
        oss << "])";
        return oss.str();
      });
}

template<typename Vec, typename CName = Vec>
void bind_readonly_vector_no_contain(py::module_ &m, const char *name) {
  py::class_<Vec>(m, name)
      .def("__len__", [](const Vec &v) { return v.size(); })
    .def("__getitem__",
         [](const Vec &v, typename Vec::size_type i) {
        if (i >= v.size()) throw py::index_error();
        return v[i];
      })
      .def("__iter__", [](const Vec &v) {
        return py::make_iterator(v.begin(), v.end());
      }, py::keep_alive<0, 1>()) // Keep vector alive while iterator exists
      .def("__repr__", [name](const Vec &v) {
        std::ostringstream oss;
        oss << name << "(";
        oss << "[";
        for (typename Vec::size_type i = 0; i < v.size(); ++i) {
          if (i) oss << ", ";
          oss << py::repr(py::cast(v[i]));
        }
        oss << "])";
        return oss.str();
      });
}
