#pragma once
#include "Module.h"
class PyTranslate : protected Module {
public:
  PyTranslate() : Module() {}
  void include(py::module_ &m);
};

extern PyTranslate py_translate;
