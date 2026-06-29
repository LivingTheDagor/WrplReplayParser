

#ifndef WTFILEUTILS_MODULE_DEF_H
#define WTFILEUTILS_MODULE_DEF_H
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

typedef void (*register_cb)(py::module_ m);

struct ModuleComponentRegister {
  ModuleComponentRegister(register_cb cb) : cb(cb) {
    next = tail;
    tail = this;
  }

  register_cb cb;
  ModuleComponentRegister *next = nullptr; // slist
  static ModuleComponentRegister *tail;
};
#endif // WTFILEUTILS_MODULE_DEF_H
