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
  bool is_server_replay = conf_blk.getBool("is_server_replay", false);
  bool source_is_linux_path = conf_blk.getBool("source_is_linux_path", false);
  auto replay_path = conf_blk.getStr("source", nullptr);
  bool bin_is_linux_path = conf_blk.getBool("bin_is_linux_path", false);
  auto bin_path = conf_blk.getStr("bin_path", nullptr);
  g_log_handler->loadSinkFromDataBlock(*conf_blk.getBlock("logging", 0));
  std::string rpl_path_str = replay_path;
  std::string bin_path_str = bin_path;
  G_UNUSED(source_is_linux_path);
  G_UNUSED(bin_is_linux_path);
#ifdef Linux
  if(!source_is_linux_path) {
    rpl_path_str = convert_os_path_to_wsl2(replay_path);
  }
  if(!bin_is_linux_path) {
    bin_path_str = convert_os_path_to_wsl2(bin_path);
  }
#endif
  std::string logfile_str = (conf_dir / "logfile.txt").string();
  initialize(bin_path_str, logfile_str);
  g_log_handler->start_thread();
  //auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  IReplayReader *rdr = nullptr;
  MemoryEfficientServerReplay *srv_rpl = nullptr;
  Replay *rpl = nullptr;
  ParserState *state_ptr = nullptr;
  if (is_server_replay) {
    fs::path t{rpl_path_str};
    srv_rpl = new MemoryEfficientServerReplay(t.string());
    rdr = srv_rpl->getRplReader();
    state_ptr = new ParserState{srv_rpl};
  } else {
    rpl = new Replay(rpl_path_str);
    rdr = rpl->getFullDecompressReplayReader();
    state_ptr = new ParserState{rpl};
  }


  auto *pkt = new ReplayPacket();
  auto start = std::chrono::high_resolution_clock::now();
  ParserState &state = *state_ptr;
  //std::exit(0);
  bool end = false;
  int AircraftCount = 0;
  uint32_t packet_count = 0;
  while (!end && rdr->getNextPacket(pkt)) {
    state.curr_time_ms = pkt->timestamp_ms;
    packet_count++;
    end = state.ParsePacket(*pkt);
  }
  auto ended = std::chrono::high_resolution_clock::now();
  for (auto &plr: state_ptr->players) {
    auto owned_eid = plr.ownedUnitRef.data;
    std::string * vehicle = nullptr;
    if (owned_eid != ecs::INVALID_ENTITY_ID) {
      vehicle = state.g_entity_mgr.getNullable<ecs::string>(owned_eid, ECS_HASH("unit__className"));
    }
    if(vehicle)
      LOGE("Name: {}; team: {}; vehicle: {}", plr.uid.data.name, plr.team.data, *vehicle);
    else
      LOGE("Name: {}; team: {}; no_vehicle", plr.uid.data.name, plr.team.data);
  }
  delete state_ptr;
  delete rdr;
  delete srv_rpl;
  delete rpl;
  delete pkt;
  std::chrono::duration<double, std::milli> duration = ended - start;
  // Output the result
  std::cout << "profile time " << duration.count() << " " << packet_count << std::endl;
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //rpl.FooterBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->debugPrintEntities();
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->getTemplateDB()->DebugPrint();
  return 0;
}