// dag_assert.h may undefine LDAG_DBGLEVEL, so read it before any include.
#if defined(LDAG_DBGLEVEL) && LDAG_DBGLEVEL > 0
constexpr bool debug_build = true;
#else
constexpr bool debug_build = false;
#endif

#include "Logger.h"
#include "dag_assert.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#if !defined(_WIN32)
#include <sys/wait.h>
#include <unistd.h>
#endif

// Issue #12: before the log handler exists, an assert or a log call must not touch it. Its accessor asserts,
// and an assert logs, so a call that touches it recursed until the stack overflowed.

static int failures = 0;

static void check(bool value, const char *what) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", what);
    failures++;
  }
}

static size_t count(const std::string &text, const std::string &part) {
  size_t n = 0;
  for (size_t at = text.find(part); at != std::string::npos; at = text.find(part, at + 1))
    n++;
  return n;
}

// A failed check before init. A release build throws AssertException, and a debug build exits with code 1. In both
// cases the failure is logged one time.
static void failed_check_before_init() {
  if (!debug_build) {
    std::ostringstream err;
    std::streambuf *old = std::cerr.rdbuf(err.rdbuf());
    bool thrown = false;
    try {
      G_CHECK(1 + 1 == 3);
    } catch (const AssertException &) {
      thrown = true;
    }
    std::cerr.rdbuf(old);
    check(thrown, "a failed check before init throws AssertException");
    check(count(err.str(), "ASSERTION FAILED") == 1, "the failed check logs one time");
    return;
  }
#if !defined(_WIN32)
  int fds[2];
  if (pipe(fds) != 0) {
    check(false, "pipe for the debug check");
    return;
  }
  pid_t pid = fork();
  if (pid == 0) {
    dup2(fds[1], 2);
    close(fds[0]);
    G_CHECK(1 + 1 == 3);
    _exit(0);
  }
  close(fds[1]);
  std::string text;
  char buf[4096];
  for (ssize_t n; (n = read(fds[0], buf, sizeof(buf))) > 0;)
    text.append(buf, size_t(n));
  close(fds[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  check(WIFEXITED(status) && WEXITSTATUS(status) == 1, "a failed check before init exits with code 1");
  check(count(text, "ASSERTION FAILED") == 1, "the failed check logs one time");
#endif
}

int main() {
  check(!g_log_handler.init_state(), "no log handler before initialize()");

  std::ostringstream err;
  std::streambuf *old = std::cerr.rdbuf(err.rdbuf());
  LOGI("info before init");
  LOGE("error before init {}", 12);
  std::cerr.rdbuf(old);

  const std::string text = err.str();
  check(text.find("info before init") == std::string::npos, "info is dropped before init");
  check(text.find("error before init 12") != std::string::npos, "an error before init goes to stderr");
  failed_check_before_init();
  check(!g_log_handler.init_state(), "logging does not create the handler");

  g_log_handler.initialize();
  g_log_handler->start_thread();
  check(g_log_handler.init_state(), "the handler exists after initialize()");
  check(log_level_allowed(DEFAULT_SINK_HANDLER, LOGLEVEL::INFO), "info passes after init");
  LOGE("error after init");
  g_log_handler->wait_until_empty();

  std::printf("logger_tests: %d failures\n", failures);
  return failures ? 1 : 0;
}
