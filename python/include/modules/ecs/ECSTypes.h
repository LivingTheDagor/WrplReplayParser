#pragma once
#include "Module.h"
#include "ecs/typesAndLimits.h"
#include "ecs/component.h"
#include "modules/bind_readonly_vector.h"

namespace ecs {
  class Component;
}
typedef py::object (*cast_cb)(const void*);
extern std::unordered_map<ecs::component_type_t, cast_cb> cast_to_py_types;
py::object castData(ecs::component_type_t hash, const void *data);
py::object castComponent(const ecs::Component *comp);

template <typename T>
py::object casterTemplate(const void*data) {
  return py::cast((const T*)data);
}
template <>
inline py::object casterTemplate<ecs::string>(const void* data) {
  ecs::string ptr = *(ecs::string*)data;
  //if(ptr.size() > 0) {
  //  ptr.resize(ptr.size()-1);
  //}
  return py::cast(ptr);
}

#define REGISTER_TYPE(T) cast_to_py_types.emplace(ecs::ComponentTypeInfo<T>::type, casterTemplate<T>)



class PyECSTypes : protected Module {
private:
  void include_array(py::module_ &m);
public:
  PyECSTypes() : Module() {}
  void include(py::module_ &m);
};



extern PyECSTypes py_ecs_types;