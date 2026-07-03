

#pragma once
#include "Module.h"

inline py::str str_to_py_str(std::string_view str) {
  py::str txt = py::reinterpret_steal<py::str>(PyUnicode_DecodeUTF8(str.data(), str.size(), "ignore"));
  return txt;
}
