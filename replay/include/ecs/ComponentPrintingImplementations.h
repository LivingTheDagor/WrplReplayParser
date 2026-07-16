

#ifndef MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
#define MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
#include <sstream>
#include "fmt/base.h"
#include "ComponentPrintingImplementationsBase.h"

template<typename T>
std::string toStringImplECS(const void * const p, int indent) {
  if constexpr (!instantiable_with<ecs::ComponentTypeInfo, T>) {
    return toStringImpl<T>(p, indent);
  }
  if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<ecs::Tag>::type)
  // tags have no data, I think this still makes the constexpr chain still clean during compilation
  {
    return std::string(ecs::ComponentTypeInfo<T>::type_name);
  } else {
    const T *data = (T *) (p);
    if constexpr (has_to_string<T>) {
      return data->toString(indent);
    }

    else if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<ecs::string>::type) {
      if (std::is_same<T, eastl::string>::value) {
        return fmt::format("\"{}\"", data->c_str()); // if I ever have ecs::string be an east::string again
      }
      return *data;
    }
    return toStringImpl<T>(data, indent);
  }
}
#endif // MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
