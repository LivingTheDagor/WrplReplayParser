#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
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

#include <csignal>
#include <cstdlib>

//#include <unistd.h>


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
  //std::signal(SIGSEGV, signal_handler);
  fs::path conf_dir = CONFIG_DIR;
  fs::path config_file = conf_dir / "dagor_replay_test.blk";
  DataBlock conf_blk;
  G_ASSERT(dblk::load(conf_blk, config_file.string().c_str()));
  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
  g_log_handler->loadSinkFromDataBlock(*conf_blk.getBlock("logging", 0));
  std::string bin_path_str = bin_path;
  G_UNUSED(bin_is_linux_path);
#ifdef _TARGET_PC_LINUX
  if(!bin_is_linux_path) {
    bin_path_str = convert_os_path_to_wsl2(bin_path);
  }
#endif
  std::string logfile_str = (conf_dir / "logfile.txt").string();
  initialize(bin_path_str, "", logfile_str);
  g_log_handler->start_thread();
  //auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  IReplayReader *rdr = nullptr;
  IReplay *rpl = nullptr;
  std::string repl_1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\Replays\#2026.05.09 03.04.41.wrpl)";
  std::string repl_2 = R"(D:\SteamLibrary\steamapps\common\War Thunder\Replays\#2026.05.09 11.16.58.wrpl)";
#ifdef _TARGET_PC_LINUX
  repl_1 = convert_os_path_to_wsl2(repl_1);
    repl_2 = convert_os_path_to_wsl2(repl_2);
#endif

    Replay rpl1{repl_1};
    rdr = rpl1.getReplayReader();
    delete rdr;


    Replay rpl2{repl_2};
    rdr = rpl1.getReplayReader();
    ReplayPacket pkt{};
    delete rdr;
  return 0;
}