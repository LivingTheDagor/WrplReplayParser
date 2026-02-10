

#ifndef MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
#define MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
#include <sstream>

// if no valid printing method found, falls back to this
template<typename T>
std::string basicPrint(void* v) {
  return fmt::format("basicPrint<{}> at {:p}", ecs::ComponentTypeInfo<T>::type_name, v);
}


template <typename T>
concept has_to_string = requires(T t, int i) {
  { t.toString(i) } -> std::convertible_to<std::string>;
};

template<typename T>
std::string toStringImpl(void *p, int indent) {
  if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<ecs::Tag>::type)
    // tags have no data, I think this still makes the constexpr chain still clean during compilation
  {
    return std::string(ecs::ComponentTypeInfo<T>::type_name);
  }
  else
  {
    const T* data = (T*)(p);
    if constexpr (has_to_string<T>) {
      return data->toString(indent);
    }

    else if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<ecs::string>::type)
    {
      return fmt::format("\"{}\"", data->c_str()); // remember, ecs::string is a eastl::string, not std::string.
    }
    else if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<float>::type)
    {
      return fmt::format("{:f}", *data);
    }
    else if constexpr (ecs::ComponentTypeInfo<T>::type == ecs::ComponentTypeInfo<bool>::type)
    {
      return fmt::format("{}", *data ? "true" : "false");
    }
    else if constexpr (HasOstreamOperator<T>) {
      std::ostringstream os;
      os << *data;
      return os.str();
    }
    else {
      // Priority 3: Fall back to basicPrint
      return basicPrint<T>(p);
    }
  }
}
#endif //MYEXTENSION_COMPONENTPRINTINGIMPLEMENTATIONS_H
