#pragma once


template <template <class...> class Z, class... Args>
concept instantiable_with = requires { typename Z<Args...>; };

template <typename T>
concept has_component_type_name = requires {
  ecs::ComponentTypeInfo<T>::type_name;
};
// if no valid printing method found, falls back to this
template<typename T>
std::string basicPrint(const void * const v) {
  if constexpr (has_component_type_name<T>) {
    return fmt::format("basicPrint<{}> at {:p}", ecs::ComponentTypeInfo<T>::type_name, v);
  } else {
    return fmt::format("basicPrint<{}> at {:p}", typeid(T).name(), v);
  }
}


template<typename T>
concept has_to_string = requires(T t, int i) {
  { t.toString(i) } -> std::convertible_to<std::string>;
};

template <typename T, typename Char = char, typename = void>
struct is_fmt_formattable : std::false_type {};

template <typename T, typename Char>
struct is_fmt_formattable<T, Char,
  std::void_t<decltype(std::declval<fmt::formatter<std::remove_cv_t<std::remove_reference_t<T>>, Char>&>()
    .format(std::declval<const std::remove_cv_t<std::remove_reference_t<T>>&>(),
            std::declval<fmt::basic_format_context<fmt::appender, Char>&>()))>> : std::true_type {};

template <typename T>
inline constexpr bool is_fmt_formattable_v = is_fmt_formattable<T>::value;


template <typename T>
using base_type_t = std::remove_pointer_t<std::remove_reference_t<T>>;
template<typename T>
std::string toStringImpl(const void * p, const int indent) {
  const T* data = static_cast<const T*>(p);
  if constexpr (has_to_string<T>) {
    return data->toString(indent);
  }
  else if constexpr (std::is_same<T, float>::value) {
    return fmt::format("{:f}", *data);
  }
  else if constexpr (std::is_same<T, bool>::value) {
    return fmt::format("{}", *data ? "true" : "false");
  }
  else if constexpr (is_fmt_formattable_v<T>) {
    return fmt::format("{}", *data);
  } else if constexpr (HasOstreamOperator<T>) {
    std::ostringstream os;
    os << *data;
    return os.str();
  } else {
    return basicPrint<T>(p);
  }
}
