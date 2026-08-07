#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include "network/CNetwork.h"
#include "FileSystem.h"
#include "init/initialize.h"
#include "mpi/mpi.h"

#include <ctime>
#include "Replay/Replay.h"
#include "Replay/find_rpl_files.h"
#include "mpi/ObjectDispatcher.h"
#include "Logger.h"

#include "state/ParserState.h"
#include <cctype>

#include <cstdlib>
#include <thread>
#include <barrier>
#include "tracy/Tracy.hpp"

//#include <unistd.h>
// ──── operator new/delete ────
void* operator new(std::size_t size) {
  void* ptr = std::malloc(size);
  if (!ptr) throw std::bad_alloc();
  TracyAllocS(ptr, size, 20);
  return ptr;
}

void operator delete(void* ptr) noexcept {
  TracyFreeS(ptr, 20);
  std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
  TracyFreeS(ptr, 20);
  std::free(ptr);
}

void* operator new[](std::size_t size) {
  void* ptr = std::malloc(size);
  if (!ptr) throw std::bad_alloc();
  TracyAllocS(ptr, size, 20);
  return ptr;
}

void operator delete[](void* ptr) noexcept {
  TracyFreeS(ptr, 20);
  std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
  TracyFreeS(ptr, 20);
  std::free(ptr);
}

std::string convert_os_path_to_wsl2(std::string &str) { // this function assumes a windows os with a wsl2 linux
  G_ASSERTF(str[1] == ':', "must be an absolute path");
  std::string payload = "/mnt/";
  payload += static_cast<char>(std::tolower(str[0]));
  payload += "/";
  payload += str.substr(3);
  std::replace(payload.begin(), payload.end(), '\\', '/');
  return payload;
}

std::string convert_os_path_to_wsl2(const char *str) {
  std::string t(str);
  return convert_os_path_to_wsl2(t);
}

//void checkMemory() {
  // Prints all live allocations (with stacks) to stderr
//  __asan_print_accumulated_stats();
//}

int main() {
  //std::signal(SIGSEGV, signal_handler);
  fs::path conf_dir = CONFIG_DIR;
  fs::path config_file = conf_dir / "dagor_replay_test.blk";
  DataBlock conf_blk;
  G_ASSERT(load(conf_blk, config_file.string().c_str()));
  bool is_server_replay = conf_blk.getBool("is_server_replay", false);
  bool source_is_linux_path = conf_blk.getBool("source_is_linux_path", false);
  auto replay_path = conf_blk.getStr("source", nullptr);
  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
  std::string rpl_path_str = replay_path;
  std::string bin_path_str = bin_path;
  G_UNUSED(source_is_linux_path);
  G_UNUSED(bin_is_linux_path);
#ifdef _TARGET_PC_LINUX
  if(!source_is_linux_path) {
    rpl_path_str = convert_os_path_to_wsl2(replay_path);
  }
  if(!bin_is_linux_path) {
    bin_path_str = convert_os_path_to_wsl2(bin_path);
  }
#endif
  std::string logfile_str = (conf_dir / "logfile.txt").string();
  initialize(bin_path_str, "", logfile_str);
  g_log_handler.start_thread();
  //auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  //checkMemory();
  ecs::g_ecs_data->printCurrentMemoryUsage();
  g_log_handler.wait_until_empty();
  g_log_handler.flush_all();
  std::string out;
  std::cin >> out;
  return 0;
}