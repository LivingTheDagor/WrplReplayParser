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
  //auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  IReplayReader *rdr = nullptr;
  IReplay *rpl = nullptr;
  if (is_server_replay) {
    fs::path t{rpl_path_str};
    rpl = new ServerReplay(t.string());
  } else {
    rpl = new Replay(rpl_path_str);
  }
  rdr = rpl->getReplayReader();
  int idx;
  auto start = std::chrono::high_resolution_clock::now();
  {
    ParserState state{rpl};
    ReplayPacket pkt{};
    //std::exit(0);

    while (rdr->getNextPacket(pkt) && state.ParsePacket(pkt)) {}
    for (auto &plr: state.players) {
      auto owned_eid = *plr.ownedUnitRef.data;
      std::string * vehicle = nullptr;
      if (owned_eid != ecs::INVALID_ENTITY_ID) {
        vehicle = state.g_entity_mgr.getNullable<ecs::string>(owned_eid, ECS_HASH("unit__className"));
      }
      if(vehicle)
        LOGE("player_id: {}; playerName: {}; team: {}; vehicle: {}", plr.uid.data->account_id, plr.uid.data->name, *plr.team.data, *vehicle);
      else
        LOGE("Name: {}; team: {}; no_vehicle", plr.uid.data->name, *plr.team.data);
    }
    iterate_all_units(state);
    idx = state.current_packet_index;
    state.rewindToMs(105620);
    state.rewindToMs(157507);
    state.rewindToMs(156777);
    delete rdr;
    delete rpl;

  }
  auto ended = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> duration = ended - start;
  // Output the result
  LOGI("Finished loading replay in {} ms, packet count: {}", duration.count(), idx);
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //rpl.FooterBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->debugPrintEntities();
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->getTemplateDB()->DebugPrint();
  return 0;
}