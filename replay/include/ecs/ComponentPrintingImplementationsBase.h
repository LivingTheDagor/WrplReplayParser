#pragma once
#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include "type_name.h"


template<template<class...> class Z, class... Args>
concept instantiable_with = requires { typename Z<Args...>; };


// Concept to check if type T supports operator<<
template<typename T>
concept HasOstreamOperator = requires(std::ostream &os, const T &t) {
  { os << t } -> std::convertible_to<std::ostream &>;
};

namespace ecs {
  template<typename T>
  struct ComponentTypeInfo;
}

template<typename T>
concept has_component_type_name = requires { ecs::ComponentTypeInfo<T>::type_name; };

template<typename T>
std::string type_name() {
  if constexpr (has_component_type_name<T>) {
    return ecs::ComponentTypeInfo<T>::type_name;
  }
  return util::type_name<T>();
}

// if no valid printing method found, falls back to this
template<typename T>
std::string basicPrint(const void *const v) {
  return fmt::format("basicPrint<{}> at {:p}", type_name<T>(), v);
}


template<typename T>
concept has_to_string = requires(T t, int i) {
  { t.toString(i) } -> std::convertible_to<std::string>;
};

template<typename T, typename Char = char, typename = void>
struct is_fmt_formattable : std::false_type {};

template<typename T, typename Char>
struct is_fmt_formattable<
  T, Char,
  std::void_t<decltype(std::declval<fmt::formatter<std::remove_cv_t<std::remove_reference_t<T>>, Char> &>().format(
    std::declval<const std::remove_cv_t<std::remove_reference_t<T>> &>(),
    std::declval<fmt::basic_format_context<fmt::appender, Char> &>()))>> : std::true_type {};

template<typename T>
inline constexpr bool is_fmt_formattable_v = is_fmt_formattable<T>::value;


template<typename T>
struct is_std_vector : std::false_type {};

template<typename U, typename Alloc>
struct is_std_vector<std::vector<U, Alloc>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_vector_v = is_std_vector<std::remove_cvref_t<T>>::value;

template<typename T>
struct is_std_array : std::false_type {};

template<typename U, size_t sz>
struct is_std_array<std::array<U, sz>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_array_v = is_std_array<std::remove_cvref_t<T>>::value;


template<class T>
constexpr bool is_described_v = boost::describe::has_describe_members<T>::value;

template<typename T>
std::string toStringImpl(const void *p, const int indent);


template<typename T>
std::string toStringImplTyped(const T *p, const int indent) {
  return toStringImpl<T>((const void *) p, indent);
}

template<typename Obj>
struct ReflectionPrinter {
  const Obj &obj;
  int indent;
  std::ostringstream &os;

  template<typename MemberDesc>
  void operator()(MemberDesc) const {
    const auto &member = obj.*(MemberDesc::pointer);
    auto str = toStringImplTyped(&member, indent + 2); // typed call
    os << fmt::format("{:>{}}{}: {}\n", "", indent + 2, MemberDesc::name, str);
  }
};

template<class T>
std::string format_reflection(const T &obj, int indent) {
  std::ostringstream os;
  using D = boost::describe::describe_members<T, boost::describe::mod_public>;

  os << type_name<T>() << " {\n";
  boost::mp11::mp_for_each<D>(ReflectionPrinter<T>{obj, indent, os});
  os << fmt::format("{:>{}}", "", indent) << "}";
  return os.str();
}


template<typename T>
using base_type_t = std::remove_pointer_t<std::remove_reference_t<T>>;
template<typename T>
std::string toStringImpl(const void *p, const int indent) {
  const T *data = static_cast<const T *>(p);
  if constexpr (has_to_string<T>) {
    return data->toString(indent);
  } else if constexpr (std::is_same<T, float>::value) {
    return fmt::format("{:f}", *data);
  } else if constexpr (std::is_same<T, bool>::value) {
    return fmt::format("{}", *data ? "true" : "false");
  } else if constexpr (is_std_vector_v<T>) {
    std::ostringstream result;
    result << fmt::format("std::vector<{}>[", type_name<typename T::value_type>());
    for (auto &v: *data) {
      result << fmt::format("\n{:>{}}", "", indent + 2);
      result << toStringImpl<base_type_t<typename T::value_type>>(&v, indent + 2);
    }
    if ((*data).size() > 0) {
      result << "\n";
    }
    result << "]";
    return result.str();
  } else if constexpr (is_std_array_v<T>) {
    std::ostringstream result;
    result << fmt::format("std::array<{}>[", type_name<typename T::value_type>());
    for (auto &v: *data) {
      result << fmt::format("\n{:>{}}", "", indent + 2);
      result << toStringImpl<base_type_t<typename T::value_type>>(&v, indent + 2);
    }
    if ((*data).size() > 0) {
      result << "\n";
    }
    result << "]";
    return result.str();
  } else if constexpr (is_fmt_formattable_v<T>) {
    return fmt::format("{}", *data);
  } else if constexpr (is_described_v<T>) {
    return format_reflection(*data, indent);
  } else if constexpr (HasOstreamOperator<T>) {
    std::ostringstream os;
    os << *data;
    return os.str();
  } else {
    return basicPrint<T>(p);
  }
}
