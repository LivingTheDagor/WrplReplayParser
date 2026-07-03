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

#include <cstdlib>
#include <thread>
#include <barrier>
#include "tracy/Tracy.hpp"



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
//#include <sanitizer/asan_interface.h>

//void checkMemory() {
  // Prints all live allocations (with stacks) to stderr
//  __asan_print_accumulated_stats();
//}

int main() {
  //std::signal(SIGSEGV, signal_handler);
  fs::path conf_dir = CONFIG_DIR;
  fs::path config_file = conf_dir / "dagor_replay_test.blk";
  DataBlock conf_blk{};
  G_ASSERT(dblk::load(conf_blk, config_file.string().c_str()));
  bool is_server_replay = conf_blk.getBool("is_server_replay", false);
  bool source_is_linux_path = conf_blk.getBool("source_is_linux_path", false);
  auto replay_path = conf_blk.getStr("source", nullptr);
  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
  g_log_handler->loadSinkFromDataBlock(*conf_blk.getBlockByNameEx("logging"));
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
  initialize(bin_path_str, logfile_str);
  // auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  constexpr int num_threads = 10;
  // checkMemory();

  auto worker = [&](int id) {
    ZoneScopedN("Worker Run");
    ParserState *state_ptr = nullptr;
    IReplay *rpl = nullptr;
    IReplayReader *rdr = nullptr;
    //sync_point.arrive_and_wait(); // block until all threads are ready
    std::cout << fmt::format("Thread {} started!\n", id);
    auto start = std::chrono::high_resolution_clock::now();
    {

      if (is_server_replay) {
        fs::path t{rpl_path_str};
        rpl = new ServerReplay(t.string());
      } else {
        rpl = new Replay(rpl_path_str);
      }
      ZoneScopedN("Create Reader and State");
      state_ptr = new ParserState(rpl);
      rdr = rpl->getReplayReader();
    }


    ParserState &state = *state_ptr;

    auto pkt = ReplayPacket();
    //std::exit(0);
    bool end = false;
    int AircraftCount = 0;
    {
      ZoneScopedN("Parsing");
      while (rdr->getNextPacket(pkt) && state.ParsePacket(pkt)) {}
    }

    delete rdr;
    delete rpl;
    auto ended = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = ended - start;
    std::cout << fmt::format("thread {} profile time: {}; packet count: {}\n", id, duration.count(),
                             state_ptr->current_packet_index);
    delete state_ptr;
  };
  for(int i = 0; i < 1; i++) {
    {
      std::vector<std::jthread> threads;
      for (int i = 0; i < num_threads; ++i)
        threads.emplace_back(worker, i);
    }
  }
  return 0;
}