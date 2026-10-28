#include "ecs/EntityManager.h"
#include "ecs/componentTypesDefs.h"
#include "network/CNetwork.h"
#include "FileSystem.h"
#include "init/initialize.h"
#include "mpi/mpi.h"

#include <ctime>
#include "Replay/Replay.h"
#include "mpi/ObjectDispatcher.h"
#include "Logger.h"

#include "state/ParserState.h"
#include <cctype>

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
int main() {

  fs::path conf_dir = CONFIG_DIR;
  g_log_handler.set_default_sink_logfile((conf_dir / "logfile.txt").string());
  g_log_handler.start_thread();
  fs::path config_file = conf_dir / "dagor_replay_test.blk";
  DataBlock conf_blk;
  G_ASSERT(load(conf_blk, config_file.string().c_str()));

  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
#ifdef _TARGET_PC_LINUX
  std::string bin_path_str = convert_os_path_to_wsl2(bin_path);
#else
  std::string bin_path_str = bin_path;
#endif
  G_UNUSED(bin_is_linux_path);
  initialize(bin_path_str);

  ParserState state{};
  return 0;
}