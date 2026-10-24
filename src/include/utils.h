
#ifndef MYEXTENSION_UTILS_H
#define MYEXTENSION_UTILS_H

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include "fmt/base.h"
#include "fmt/format.h"
#include <iostream>
#include <sstream>
#include <span>
#include "cpptrace_compat.h"
extern bool DO_VERBOSE;

#define MAKE4C(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
// A multichar literal has its first char in the high byte. _MAKE4C('VRFs') gives the uint32 that the 4 bytes
// "VRFs" make when you read them little-endian from a file.
#define _MAKE4C(x)         MAKE4C((int(x) >> 24) & 0xFF, (int(x) >> 16) & 0xFF, (int(x) >> 8) & 0xFF, int(x) & 0xFF)


class ExceptionException : public std::runtime_error {
public:
  explicit ExceptionException(const std::string &msg) : std::runtime_error(msg) {}

  const char *what() const noexcept override { return std::runtime_error::what(); }
};

[[noreturn]] void fatal(const char *file, int line, const char *function, std::string message);


#define EXCEPTION(format_, ...) fatal(__FILE__, __LINE__, __FUNCTION__, fmt::format(format_ __VA_OPT__(, ) __VA_ARGS__))


/// Given a stream and a buffer, will attempt to write python like bytes to it
/// \param oss
/// \param buff
inline void FormatBytesToStream(std::basic_ostream<char> &oss, std::span<char> buff) {
  for (auto c: buff) {
    unsigned char v = (unsigned char) c;
    if (std::isprint(v)) {
      oss.write((const char *) &v, 1);
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

inline void FormatHexToStream(std::basic_ostream<char> &oss, std::span<const char> buff) {
  for (char c: buff) {
    oss << fmt::format("{:02x}", (unsigned char) c);
  }
}

inline std::ostringstream FormatHexToStream(std::span<const char> buff) {
  std::ostringstream oss{};
  FormatHexToStream(oss, buff);
  return oss;
}

inline std::ostringstream FormatHexToStream(std::span<const uint8_t> buff) {
  return FormatHexToStream(std::span<const char>(reinterpret_cast<const char *>(buff.data()), buff.size()));
}

#endif // MYEXTENSION_UTILS_H
