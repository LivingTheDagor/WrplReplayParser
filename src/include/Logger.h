

#ifndef LOGGER_LOGGER_H
#define LOGGER_LOGGER_H

#include <condition_variable>
#include <fstream>
#include <unordered_map>
#include <utility>
#include <cassert>
#include "mutex"
#include "string"
#include "array"
#include "atomic"
#include "iostream"
#include "queue"
#include "memory"

#include "fmt/base.h"
#include "fmt/format.h"

//internal handle for a specific sink, you get a sink handle when you create or query a sink
typedef int16_t sink_handle_t;
// represents default handler, will output to console
static constexpr sink_handle_t DEFAULT_SINK_HANDLER = -1;
// represents an invalid handle, any log calls with this handle will not be logged
static constexpr sink_handle_t INVALID_SINK_HANDLER = -2;
typedef uint8_t DEBUG_LEVEL;

uint64_t get_current_time_ms();


enum LOGLEVEL : uint8_t {
  INFO,
  DEBUG_L1, // debug level 1, most basic messages
  DEBUG_L2, // debug level 2, for messages that can be optionally disabled
  DEBUG_L3, // debug level 3, for messages that can be optionally disabled
  ERROR_, // when an error occurs
};

class log_handler;

class file_sink {
  friend log_handler;
  std::string file_sink_path;
  std::ofstream out;
public:
  ~file_sink();

  file_sink(const std::string &path) : file_sink_path(path), out(path) {
    if (!out.is_open()) {
      throw std::runtime_error("Failed to open log file: " + path);
    }
  }

  void onMessage(const std::string_view msg) {
    out << msg;
  }

  inline void flush() {
    out.flush();
  }

};


struct log_msg {
  std::string msg;
  LOGLEVEL lvl;
  sink_handle_t sink_handle;
  uint64_t time_ms;

  // Parameterized constructor
  log_msg(std::string message, LOGLEVEL level, sink_handle_t handle, uint64_t time)
      : msg(std::move(message)), lvl(level), sink_handle(handle), time_ms(time) {}

  log_msg(std::string &&message, LOGLEVEL level, sink_handle_t handle, uint64_t time)
      : msg(std::move(message)), lvl(level), sink_handle(handle), time_ms(time) {}

  log_msg() = default;

  log_msg(const log_msg &) = default;

  log_msg(log_msg &&) = default;

  log_msg &operator=(const log_msg &) = default;

  log_msg &operator=(log_msg &&) = default;

  ~log_msg() = default;
};


class logger_sink {
  friend log_handler;
  // these settings are what the default sink uses
  bool print_to_console = true;
  DEBUG_LEVEL level = 3;
  std::shared_ptr<file_sink> f_sync;
  //sink_handler_t handle = DEFAULT_SINK_HANDLER;
  uint64_t start_time_ms;
  std::string name;

  logger_sink(std::string name, uint64_t start_time) : start_time_ms(start_time), name(std::move(name)) {}

  logger_sink(std::string name, uint64_t start_time, bool print) : start_time_ms(start_time), name(std::move(name)), print_to_console(print) {}


  logger_sink(const std::string &name, bool print, std::shared_ptr<file_sink> sync/*, sink_handler_t handle*/,
              DEBUG_LEVEL lvl, uint64_t start) {
    this->name = name;
    this->f_sync = std::move(sync);
    //this->handle = handle;
    this->level = lvl;
    this->print_to_console = print;
    this->start_time_ms = start;
  }

  void on_message(log_msg &msg) const {
    double time = ((double) (msg.time_ms - this->start_time_ms)) / 1000.0;
    std::string m;
    switch (msg.lvl) {
      case INFO: {
        m = fmt::format("{:.3f} [I]  {}\n", time, msg.msg);
        break;
      }
      case DEBUG_L1: {
        if (this->level == 0)
          return;
        m = fmt::format("{:.3f} [D1] {}\n", time, msg.msg);
        break;
      }
      case DEBUG_L2: {
        if (this->level < 2)
          return;
        m = fmt::format("{:.3f} [D2] {}\n", time, msg.msg);
        break;
      }
      case DEBUG_L3: {
        if (this->level < 3)
          return;
        m = fmt::format("{:.3f} [D2] {}\n", time, msg.msg);
        break;
      }
      case ERROR_: {
        m = fmt::format("{:.3f} [E]  {}\n", time, msg.msg);
        break;
      }
    }
    // at this point, we should always have a message
    if (this->print_to_console)
      std::cout << m;
    if (this->f_sync)
      this->f_sync->onMessage(m);
  }

  void flush(bool flush_con = false) {
    if (flush_con && this->print_to_console)
      std::cout.flush();
    if (this->f_sync)
      this->f_sync->flush();
  }
};


class log_handler {

  friend file_sink;

  std::queue<log_msg> messages{};
  std::mutex queue_access_mtx{};
  std::atomic_bool running{true};
  std::condition_variable cv;
  std::mutex cv_mtx;
  std::mutex sink_access_mtx;
  uint64_t init_time = get_current_time_ms();
  std::vector<log_msg> msgs{}; //only used during consumer loop
  logger_sink default_sink;
  std::vector<logger_sink *> sinks{};

  std::unordered_map<std::string, std::weak_ptr<file_sink>> file_sinks{};
  std::thread logger_thread;
  bool thread_exists = false;
  bool destructing = false;
  //pthread_t handle; // handle for consumer thread

  logger_sink *get_sink(sink_handle_t s_handle) {
    switch (s_handle) {
      case INVALID_SINK_HANDLER:
        return nullptr;
      case DEFAULT_SINK_HANDLER:
        return &default_sink;
      default:
        if (s_handle > -1 && s_handle < (sink_handle_t) this->sinks.size())
          return this->sinks[s_handle];
        return nullptr;
    }
  }


  void action() {
    {
      std::lock_guard<std::mutex> lock(queue_access_mtx);
      msgs.clear();
      msgs.reserve(messages.size());
      while (!messages.empty()) {
        msgs.push_back(messages.front());
        messages.pop();
      }
    }
    for (auto &msg: msgs) {
      auto sink = get_sink(msg.sink_handle);
      if (sink)
        sink->on_message(msg);
    }
    msgs.clear();
  }

  void consumer_loop() {
    while (running) {
      if (!messages.empty()) {
        action();
      } else {
        std::unique_lock<std::mutex> lock(cv_mtx);
        cv.wait_for(lock, std::chrono::milliseconds(100), [&] {
          return !messages.empty() || !running;
        });
      }
      cv.notify_all();
    }
  }

  bool remove_file_sink(std::string &file_name) {
    this->file_sinks.erase(
        file_name); // this, for now, will only be called on a file_sink dtor, so obviously the file sink must already exist
    return true;
  }

  std::shared_ptr<file_sink> get_file_sink(std::string_view file_name) {
    std::string f(file_name);
    auto iter = this->file_sinks.find(f);
    if (iter != this->file_sinks.end()) {
      assert(!iter->second.expired());
      return iter->second.lock();
    }
    std::shared_ptr<file_sink> sink = std::make_shared<file_sink>(f);
    this->file_sinks.emplace(file_name, sink);
    return sink;
  }

public:

  void start_thread() {
    if(!this->logger_thread.joinable()) {
      this->logger_thread = std::thread(&log_handler::consumer_loop, this);
      this->thread_exists = true;
    }
  }

  void wait_until_empty() {
    if(!this->thread_exists)
      return;
    std::unique_lock<std::mutex> lock(cv_mtx);
    size_t len = 0;
    do {
      {
        std::lock_guard<std::mutex> lock2(this->queue_access_mtx);
        len = this->messages.size();
      }
      cv.wait(lock);
    } while(len != 0);
  }


  void flush_all() {
    for (auto sink: sinks) {
      sink->flush();
      delete sink;
    }
    std::cout.flush();
  }

  ~log_handler() {
    this->destructing = true;
    this->running = false;
    logger_thread.join();
    this->thread_exists = false;

    action(); // do final cleanup, only allow because logger thread has been destroyed
    flush_all();
  }

  sink_handle_t get_sink(const std::string &name) {
    if (name == "default")
      return DEFAULT_SINK_HANDLER;
    std::lock_guard<std::mutex> lock(this->sink_access_mtx);
    for (sink_handle_t i = 0; i < (sink_handle_t) this->sinks.size(); i++) {
      auto sink = this->sinks[i];
      if (sink && sink->name == name)
        return i;
    }
    return INVALID_SINK_HANDLER;
  }

  bool sink_dbg_lvl_allowed(sink_handle_t handle_, LOGLEVEL lvl) {
    auto sink = this->get_sink(handle_);
    switch (lvl) {
      case INFO:
        if(sink)
          return true;
      case DEBUG_L1:
        if(sink && sink->level >= 1)
          return true;
      case DEBUG_L2:
        if(sink && sink->level >= 2)
          return true;
      case DEBUG_L3:
        if(sink && sink->level >= 3)
          return true;
      case ERROR_:
        return true; // we always log error messages, if the sink doesnt exist, we log to default
    }
  }

  void sink_flush(sink_handle_t s_handle) {
    if (s_handle == INVALID_SINK_HANDLER)
      return;
    auto sink = get_sink(s_handle);
    if (sink) {
      //action();
      sink->flush(true);
    }
  }

  log_handler() : default_sink("default", this->init_time, true) {}

  void add_logmessage(std::string &&message, LOGLEVEL lvl, sink_handle_t handler) {
    if (handler == INVALID_SINK_HANDLER) // invalid handle, message wont be added to log queue
    {
      if(lvl == LOGLEVEL::ERROR_)
        handler = DEFAULT_SINK_HANDLER; // we still want errors to be logged
      else {
        return;
      }
    }
    log_msg msg{message, lvl, handler, get_current_time_ms()};
    if(this->thread_exists) // means thread exists
    {
      std::lock_guard<std::mutex> lock(queue_access_mtx);
      messages.emplace(std::move(msg));
    }
    else { // we log it now
      auto sink = get_sink(msg.sink_handle);
      if (sink)
        sink->on_message(msg);
    }
  }

  sink_handle_t add_sink(const std::string &name, bool print_to_console, bool time_from_start,
                         DEBUG_LEVEL dbg_level = 3 /*max_level*/, std::string_view file_name = "") {
    std::shared_ptr<file_sink> f_sink = nullptr;
    std::lock_guard<std::mutex> lock(this->sink_access_mtx);
    if (!file_name.empty())
      f_sink = this->get_file_sink(file_name);
    auto handler = new logger_sink(name, print_to_console, f_sink, dbg_level,
                                   time_from_start ? this->init_time : get_current_time_ms());
    this->sinks.emplace_back(handler);
    return this->sinks.size() - 1;
  }

  void set_default_sink_logfile(const std::string &file_name)  {
    std::lock_guard<std::mutex> lock(this->sink_access_mtx);
    std::shared_ptr<file_sink> f_sink = this->get_file_sink(file_name);
    this->default_sink.f_sync = f_sink;
  }
};

extern log_handler g_log_handler;

void log_ext(const std::string &func, int line, sink_handle_t sink, LOGLEVEL level, std::string &&message);

#define LOG_FMT_EXT(sink, level, format_, ...) \
    (log_ext(__FUNCTION__, __LINE__, sink, level, fmt::format(format_ __VA_OPT__(, ) __VA_ARGS__)))

#define LOG_FMT_EXT_CHECK(sink, level, format_, ...) \
  if(g_log_handler.sink_dbg_lvl_allowed(sink, level)) {                                                 \
    (log_ext(__FUNCTION__, __LINE__, sink, level, fmt::format(format_ __VA_OPT__(, ) __VA_ARGS__)))     \
  }


#define LOGI(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::INFO, format_, __VA_ARGS__)
#define LOGD(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::DEBUG_L1, format_, __VA_ARGS__)
#define LOGD1(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::DEBUG_L1, format_, __VA_ARGS__)
#define LOGD2(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::DEBUG_L2, format_, __VA_ARGS__)
#define LOGD3(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::DEBUG_L3, format_, __VA_ARGS__)
#define LOGE(format_, ...) LOG_FMT_EXT(DEFAULT_SINK_HANDLER, LOGLEVEL::ERROR_, format_, __VA_ARGS__)

#define ELOGI(sink, format_, ...) LOG_FMT_EXT_CHECK(sink, LOGLEVEL::INFO, format_, __VA_ARGS__)
#define ELOGD(sink, format_, ...) LOG_FMT_EXT_CHECK(sink, LOGLEVEL::DEBUG_L1, format_, __VA_ARGS__)
#define ELOGD1(sink, format_, ...) LOG_FMT_EXT_CHECK(sink, LOGLEVEL::DEBUG_L1, format_, __VA_ARGS__)
#define ELOGD2(sink, format_, ...) LOG_FMT_EXT_CHECK(sink, LOGLEVEL::DEBUG_L2, format_, __VA_ARGS__)
#define ELOGD3(sink, format_, ...) LOG_FMT_EXT_CHECK(sink, LOGLEVEL::DEBUG_L3, format_, __VA_ARGS__)
#define ELOGE(sink, format_, ...) LOG_FMT_EXT(sink, LOGLEVEL::ERROR_, format_, __VA_ARGS__)


#define LOG LOGI


#endif //LOGGER_LOGGER_H


