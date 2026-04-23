#pragma once
#include "Module.h"

class PyECSQueryES : protected Module {
public:
  PyECSQueryES() : Module() {}
  void include(py::module_ &m);
};

extern PyECSQueryES py_ecs_query_es;
