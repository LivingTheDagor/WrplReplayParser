#pragma once

#include <memory>
#include <string>
#include <typeinfo>
#include <cstdlib>

#if defined(__has_include)
  #if __has_include(<cxxabi.h>)
    #include <cxxabi.h>
    #define TYPE_NAME_HAS_CXA_DEMANGLE 1
  #endif
#endif

namespace util {

  namespace detail {
    inline std::string demangle_or_fallback(const char* name) {
#if defined(TYPE_NAME_HAS_CXA_DEMANGLE)
      int status = 0;
      std::unique_ptr<char, void(*)(void*)> demangled(
          abi::__cxa_demangle(name, nullptr, nullptr, &status),
          std::free
      );
      if (status == 0 && demangled) return std::string(demangled.get());
#endif
      return std::string(name);
    }
  } // namespace detail

  template <typename T>
  std::string type_name() {
    return detail::demangle_or_fallback(typeid(T).name());
  }

  template <typename T>
  std::string type_name_of_obj(const T * obj) {
    return detail::demangle_or_fallback(typeid(*obj).name());
  }

  template <typename T>
  std::string type_name_of(const T&) {
    return type_name<T>();
  }

} // namespace util