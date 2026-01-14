//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once
#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <fmt/base.h>
#include <iostream>
#include <cstdlib>
#include <cstdarg>  // for va_list, va_start, va_end
#include <cstdint>
//#include <cpptrace/cpptrace.hpp>
#include "utils.h"

#ifdef __analysis_assume
#define G_ANALYSIS_ASSUME __analysis_assume
#else
#define G_ANALYSIS_ASSUME(expr)
#endif

#ifndef DAGOR_LIKELY
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
#if defined(__cplusplus)
#define DAGOR_LIKELY(x)   __builtin_expect(!!(x), true)
#define DAGOR_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define DAGOR_LIKELY(x)   __builtin_expect(!!(x), 1)
#define DAGOR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#else
#define DAGOR_LIKELY(x)   (x)
#define DAGOR_UNLIKELY(x) (x)
#endif
#endif
[[noreturn]] inline void
assert_failed_ext(const char *file, int line, const char *function, const char *expression, std::string message) {
  std::cerr << "ASSERT FAILED CERR\n";
  //std::fprintf(stderr, "ASSERTION FAILED:\n %s:%d\nFunction: %s \nExpression: %s \n", file, line, function, expression);
  LOGE("ASSERTION FAILED:\n {}:{}\nFunction: {} \nExpression: {} \n", file, line, function, expression);
  if (!message.empty()) {
    LOGE("Message: {}", message);
  }
  //LOGE("{}", cpptrace::generate_trace().to_string());
  g_log_handler.wait_until_empty();
  g_log_handler.flush_all();
  std::exit(EXIT_FAILURE);
}

#define assert_failed(file, line, func, expr_str, format_, ...) assert_failed_ext(file, line, func, expr_str, fmt::format(format_ __VA_OPT__(, ) __VA_ARGS__))


#define DAGOR_DBGLEVEL 2
#if DAGOR_DBGLEVEL < 1
// No side-effects in release
#define G_ASSERT_EX(expression, expr_str)            ((void)0)
#define G_ASSERTF_EX(expression, expr_str, fmt, ...) ((void)0)
#define G_ASSERT(expression)                         ((void)0)
#define G_ASSERTF(expression, fmt, ...)              ((void)0)
#define G_ASSERTF_ONCE(expression, fmt, ...)         ((void)0)
#define G_FAST_ASSERT(expression)                    ((void)0)
#define G_ASSERT_FAIL(fmt, ...)                      ((void)0)
#else
#define G_ASSERT_EX(expression, expr_str)                                 \
  do                                                                      \
  {                                                                       \
    const bool g_assert_result_ = !!(expression);                         \
    if (DAGOR_UNLIKELY(!g_assert_result_))                                \
      assert_failed_ext(__FILE__, __LINE__, __FUNCTION__, expr_str, ""); \
  } while (0)

#define G_ASSERTF_EX(expression, expr_str, fmt, ...)                                   \
  do                                                                                   \
  {                                                                                    \
    const bool g_assert_result_ = !!(expression);                                      \
    if (DAGOR_UNLIKELY(!g_assert_result_))                                             \
        assert_failed(__FILE__, __LINE__, __FUNCTION__, expr_str, fmt, ##__VA_ARGS__); \
  } while (0)

#define G_ASSERT_FAIL(fmt, ...)                                                    \
  {                                                                                \
    assert_failed(__FILE__, __LINE__, __FUNCTION__, expr_str, fmt, ##__VA_ARGS__); \
  }

#define G_ASSERTF_ONCE(expression, fmt, ...)                                         \
  do                                                                                 \
  {                                                                                  \
    static bool showed_ = false;                                                     \
    const bool g_assert_result_ = !!(expression);                                    \
    if (DAGOR_UNLIKELY(!g_assert_result_) && !showed_)                               \
    {                                                                                \
      assert_failed(__FILE__, __LINE__, __FUNCTION__, expr_str, fmt, ##__VA_ARGS__); \
      showed_ = true;                                                                \
    }                                                                                \
  } while (0)

#define G_ASSERT(expression)            G_ASSERT_EX(expression, #expression)
#define G_ASSERTF(expression, fmt, ...) G_ASSERTF_EX(expression, #expression, fmt, ##__VA_ARGS__)

// This assertion API is faster because it's won't do any function calls within, therefore not translating
// functions that call it in non-leaf functions (and preventing optimizer to inline it for example)
// It's also smaller in terms of code size: on x86[_64] it's only 3 (test+jmp+int3) instructions long
// Downside of it - no pretty message box with message when something gone wrong (only standard Unhandled exception/Segfault one)
// It's programmer's job to choose wisely which one to use for each specific case (but generally it's better to stick to G_ASSERT)
#if DAGOR_DBGLEVEL > 1
#define G_FAST_ASSERT G_ASSERT
#else
#define G_FAST_ASSERT(expr)      \
  do                             \
  {                              \
    if (DAGOR_UNLIKELY(!(expr))) \
      G_DEBUG_BREAK_FORCED;      \
  } while (0)
#endif
#endif

// _LOG functions check the condition and produce error messages even
// in production builds, which allows them to be reported. Use with care,
// bandwidth costs money!
#if DAGOR_DBGLEVEL < 1
#define G_ASSERT_LOG(expression, ...)  \
  do                                   \
  {                                    \
    if (DAGOR_UNLIKELY(!(expression))) \
      logerr(__VA_ARGS__);             \
  } while (0)
// Does and action before logging. Useful for providing additional
// context about what went worng.
#define G_ASSERT_DO_AND_LOG(expression, action, ...) \
  do                                                 \
  {                                                  \
    if (DAGOR_UNLIKELY(!(expression)))               \
    {                                                \
      action;                                        \
      logerr(__VA_ARGS__);                           \
    }                                                \
  } while (0)
#else
#define G_ASSERT_LOG(expression, ...) G_ASSERTF(expression, ##__VA_ARGS__)
#define G_ASSERT_DO_AND_LOG(expr, action, ...)               \
  do                                                         \
  {                                                          \
    const bool g_assert_result_do_ = !!(expr);               \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))                \
      action;                                                \
    G_ASSERTF_EX(g_assert_result_do_, #expr, ##__VA_ARGS__); \
  } while (0)
#endif

#if DAGOR_DBGLEVEL < 1
#define G_VERIFY(expression) \
  do                         \
  {                          \
    (void)(expression);      \
  } while (0)
#define G_VERIFYF(expression, fmt, ...) \
  do                                    \
  {                                     \
    (void)(expression);                 \
  } while (0)
#else
#define G_VERIFY(expression)                                          \
  do                                                                  \
  {                                                                   \
    bool g_verify_result_ = !!(expression);                           \
    if (DAGOR_UNLIKELY(!g_verify_result_))                            \
      assert_failed_ext(__FILE__, __LINE__, __FUNCTION__, nullptr, ""); \
  } while (0)

#define G_VERIFYF(expression, fmt, ...)                                              \
  do                                                                                 \
  {                                                                                  \
    bool g_verify_result_ = !!(expression);                                          \
    G_ANALYSIS_ASSUME(g_verify_result_);                                             \
    if (DAGOR_UNLIKELY(!g_verify_result_))                                           \
      assert_failed(__FILE__, __LINE__, __FUNCTION__, #expression, fmt, ##__VA_ARGS__); \
  } while (0)
#endif


#if !defined(G_STATIC_ASSERT)
#define G_STATIC_ASSERT(x) static_assert((x), "assertion failed: " #x)
#endif


// Warning: this macro is unhygienic! Use with care around ifs and loops.
#define G_ASSERT_AND_DO_UNHYGIENIC(expr, cmd)  \
  {                                            \
    const bool g_assert_result_do_ = !!(expr); \
    G_ASSERT_EX(g_assert_result_do_, #expr);   \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))  \
      cmd;                                     \
  }

// Warning: this macro is unhygienic! Use with care around ifs and loops.
#define G_ASSERTF_AND_DO_UNHYGIENIC(expr, cmd, fmt, ...)          \
  {                                                               \
    const bool g_assert_result_do_ = !!(expr);                    \
    G_ASSERTF_EX(g_assert_result_do_, #expr, fmt, ##__VA_ARGS__); \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))                     \
      cmd;                                                        \
  }

#define G_ASSERT_AND_DO(expr, cmd)        \
  do                                      \
    G_ASSERT_AND_DO_UNHYGIENIC(expr, cmd) \
  while (0)

#define G_ASSERTF_AND_DO(expr, cmd, fmt, ...)                  \
  do                                                           \
    G_ASSERTF_AND_DO_UNHYGIENIC(expr, cmd, fmt, ##__VA_ARGS__) \
  while (0)

#define G_ASSERT_RETURN(expr, returnValue) G_ASSERT_AND_DO(expr, return returnValue)
#define G_ASSERT_BREAK(expr)               G_ASSERT_AND_DO_UNHYGIENIC(expr, break)
#define G_ASSERT_CONTINUE(expr)            G_ASSERT_AND_DO_UNHYGIENIC(expr, continue)

#define G_ASSERTF_RETURN(expr, returnValue, fmt, ...) G_ASSERTF_AND_DO(expr, return returnValue, fmt, ##__VA_ARGS__)
#define G_ASSERTF_BREAK(expr, fmt, ...)               G_ASSERTF_AND_DO_UNHYGIENIC(expr, break, fmt, ##__VA_ARGS__)
#define G_ASSERTF_CONTINUE(expr, fmt, ...)            G_ASSERTF_AND_DO_UNHYGIENIC(expr, continue, fmt, ##__VA_ARGS__)

#if DAGOR_DBGLEVEL > 0
#define G_DEBUG_BREAK G_DEBUG_BREAK_FORCED
#else
#define G_DEBUG_BREAK ((void)0)
#endif

#undef DAGOR_DBGLEVEL

#define G_LOGERR_AND_DO(expression, action, ...) \
  if (DAGOR_UNLIKELY(!(expression)))             \
  {                                              \
    logerr(__VA_ARGS__);                         \
    action;                                      \
  }                                              \
  else


