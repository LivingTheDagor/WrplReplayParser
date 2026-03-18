#include "fmt/base.h"
#include "logger.h"
#include "thread"
#include <chrono>
log_handler g_log_handler;

file_sink::~file_sink() {
  out.close();
  if(!g_log_handler.destructing)
    g_log_handler.remove_file_sink(this->file_sink_path);
}

uint64_t get_current_time_ms() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void log_ext(const std::string &func, int line, sink_handle_t sink, LOGLEVEL level, std::string &&message) {
  g_log_handler.add_logmessage(std::move(fmt::format("{}({}): {}", func, line, message)), level, sink);
}


#include <cpptrace/cpptrace.hpp>
#ifdef WIN32
#include <windows.h>
// Your custom handler
LONG WINAPI my_handler(EXCEPTION_POINTERS* ExceptionInfo) {
// You can filter by exception type:
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    LOGE("SIGSEV occured, stacktrace: \n{}", cpptrace::generate_trace().to_string());
    g_log_handler.wait_until_empty();
    g_log_handler.flush_all();
  }
// Print your stacktrace here...
// Optionally: exit, abort, etc.
  return EXCEPTION_EXECUTE_HANDLER;
}

// Call this function during initialization
void register_default_sigsev_handler() {
  SetUnhandledExceptionFilter(my_handler);
}
#else
#include "signal.h"

static void handler(int signum)
{
  LOGE("SIGSEV occured, stacktrace: \n{}", cpptrace::generate_trace().to_string());
  g_log_handler.wait_until_empty();
  g_log_handler.flush_all();
  std::exit(1);
  /* Take appropriate actions for signal delivery */
}

void register_default_sigsev_handler() {
  struct sigaction sa;


  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* Restart functions if
                                 interrupted by handler */
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    std::cerr << "error installing SIGSEGV handler" << std::endl;
}

#endif



