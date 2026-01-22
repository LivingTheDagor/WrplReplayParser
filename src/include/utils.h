
#ifndef MYEXTENSION_UTILS_H
#define MYEXTENSION_UTILS_H

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "fmt/base.h"
#include "fmt/format.h"
#include <iostream>
#include <cstdlib>
#include <cstdarg>  // for va_list, va_start, va_end
#include <cstdint>
#include <sstream>
#include <span>
#include "Logger.h"
//#include <cpptrace/cpptrace.hpp>

extern bool DO_VERBOSE;

#define MAKE4C(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define _MAKE4C(x) MAKE4C((int(x) >> 24) & 0xFF, (int(x) >> 16) & 0xFF, (int(x) >> 8) & 0xFF, int(x) & 0xFF)

[[noreturn]] inline void fatal(const char *file, int line, const char *function, std::string message) {
  std::cerr << "fatal CERR\n";
  LOGE("Fatal error at {}:{}\nFunction: {} \nMessage: {}", file, line, function, message);
  //LOGE("{}", cpptrace::generate_trace().to_string());
  g_log_handler.wait_until_empty();
  g_log_handler.flush_all();
  std::exit(EXIT_FAILURE);
}


//#define LOG(...) log(__VA_ARGS__)


#define VERBOSE_LOG(...)  \
{                         \
  if(DO_VERBOSE)          \
    log(__VA_ARGS__);     \
}

#define EXCEPTION(format_, ...) fatal(__FILE__, __LINE__, __FUNCTION__, fmt::format(format_ __VA_OPT__(, ) __VA_ARGS__))

#define EXCEPTION_IF_FALSE(cond, ...) \
    do { \
        if (!(cond)) \
            EXCEPTION(__VA_ARGS__); \
    } while(0)

inline int popcount(uint32_t val) {
#ifdef _MSC_VER
  return _mm_popcnt_u32(val);
#else
  return std::__popcount(val);
#endif
}

#define G_UNUSED(x)    ((void)(x))
#define G_UNREFERENCED G_UNUSED

struct Version {
private:
  uint8_t data[4];
public:

};

/// Given a stream and a buffer, will attempt to write python like bytes to it
/// \param oss
/// \param buff
inline void FormatBytesToStream(std::basic_ostream<char> &oss, std::span<char> buff) {
  for (auto c: buff) {
    if (std::isprint(c)) {
      oss.write((const char *) &c, 1);
    } else {
      oss << fmt::format("\\x{:02X}", c);
    }
  }
}

inline void FormatOnlyTextToStream(std::basic_ostream<char> &oss, std::span<char> buff) {
  for (auto c: buff) {
    if (std::isprint(c)) {
      oss.write((const char *) &c, 1);
    }
  }
}

inline void FormatHexToStream(std::basic_ostream<char> &oss, std::span<char> buff) {
  for (char c : buff) {
    oss << fmt::format("{:02x}", (unsigned char)c);
  }
}

#endif //MYEXTENSION_UTILS_H
