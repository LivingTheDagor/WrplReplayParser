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
#ifdef _MSC_VER
#include <io.h>
#define write _write
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#else
#include <unistd.h>
#endif
#include <signal.h>
#include <string.h>
// Your custom handler
LONG WINAPI my_handler(EXCEPTION_POINTERS* ExceptionInfo) {
// You can filter by exception type:
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    const char msg[] = "SIGSEGV detected what did you do\n";
    write(STDERR_FILENO, msg, sizeof(msg)-1);
    auto stk = cpptrace::generate_trace().to_string();
    write(STDERR_FILENO, stk.c_str(), strlen(stk.c_str()));
    _exit(129);

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
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <string.h>

#include <sys/wait.h>
#include <cstring>
#include <signal.h>
#include <unistd.h>

// from signal_demo.cpp
// This is just a utility I like, it makes the pipe API more expressive.
struct pipe_t {
  union {
    struct {
      int read_end;
      int write_end;
    };
    int data[2];
  };
};

void do_signal_safe_trace_local(cpptrace::frame_ptr* buffer, std::size_t count) {
  // Setup pipe and spawn child
  pipe_t input_pipe;
  pipe(input_pipe.data);
  const pid_t pid = fork();
  if(pid == -1) {
    const char* fork_failure_message = "fork() failed\n";
    write(STDERR_FILENO, fork_failure_message, strlen(fork_failure_message));
    return;
  }
  if(pid == 0) { // child
    dup2(input_pipe.read_end, STDIN_FILENO);
    close(input_pipe.read_end);
    close(input_pipe.write_end);
    execl("signal_tracer", "signal_tracer", nullptr);
    const char* exec_failure_message = "exec(signal_tracer) failed: Make sure the signal_tracer executable is in "
                                       "the current working directory and the binary's permissions are correct.\n";
    write(STDERR_FILENO, exec_failure_message, strlen(exec_failure_message));
    _exit(1);
  }
  // Resolve to safe_object_frames and write those to the pipe
  for(std::size_t i = 0; i < count; i++) {
    cpptrace::safe_object_frame frame;
    cpptrace::get_safe_object_frame(buffer[i], &frame);
    write(input_pipe.write_end, &frame, sizeof(frame));
  }
  close(input_pipe.read_end);
  close(input_pipe.write_end);
  // Wait for child
  waitpid(pid, nullptr, 0);
}

void handler(int signo, siginfo_t* info, void* context) {
  // Print basic message
  const char* message = "SIGSEGV occurred:\n";
  write(STDERR_FILENO, message, strlen(message));
  // Generate trace
  constexpr std::size_t N = 100;
  cpptrace::frame_ptr buffer[N];
  std::size_t count = cpptrace::safe_generate_raw_trace(buffer, N);
  do_signal_safe_trace_local(buffer, count);
  // Up to you if you want to exit or continue or whatever
  _exit(1);
}

void warmup_cpptrace() {
  // This is done for any dynamic-loading shenanigans
  cpptrace::frame_ptr buffer[10];
  std::size_t count = cpptrace::safe_generate_raw_trace(buffer, 10);
  cpptrace::safe_object_frame frame;
  cpptrace::get_safe_object_frame(buffer[0], &frame);
}
void register_default_sigsev_handler() {
  warmup_cpptrace();
  // Setup signal handler
  struct sigaction action = { 0 };
  action.sa_flags = 0;
  action.sa_sigaction = &handler;
  if (sigaction(SIGSEGV, &action, NULL) == -1) {
    perror("sigaction");
  }
  if (sigaction(SIGABRT, &action, NULL) == -1) {
    perror("sigaction");
  }

  /// ...
}

#endif



