#include "fmt/base.h"
#include "logger.h"
#include "thread"
log_handler g_log_handler;

file_sink::~file_sink() {
  out.close();
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



