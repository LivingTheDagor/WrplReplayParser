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
  payload += std::tolower(str[0]);
  payload += "/";
  payload += str.substr(3);
  std::replace(payload.begin(), payload.end(), '\\', '/');
  return std::move(payload);
}

std::string convert_os_path_to_wsl2(const char *str) {
  std::string t(str);
  return convert_os_path_to_wsl2(t);
}


int main()
{
  //std::signal(SIGSEGV, signal_handler);
  fs::path conf_dir = CONFIG_DIR;
  g_log_handler.set_default_sink_logfile((conf_dir / "logfile.txt").string());
  g_log_handler.start_thread();
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
#ifdef Linux
  if(!source_is_linux_path) {
    rpl_path_str = convert_os_path_to_wsl2(replay_path);
  }
  if(!bin_is_linux_path) {
    bin_path_str = convert_os_path_to_wsl2(bin_path);
  }
#endif
  initialize(bin_path_str);
  auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate("attachable_wear_fast_sf_helmet_item");
  IReplayReader *rdr = nullptr;
  ServerReplay *srv_rpl = nullptr;
  Replay *rpl = nullptr;
  if(is_server_replay)
  {
    fs::path t{rpl_path_str};
    srv_rpl = new ServerReplay(t);
    rdr = srv_rpl->getRplReader();
  }
  else
  {
     rpl = new Replay(rpl_path_str);
    rdr = rpl->getRplReader();

  }


  auto *pkt = new ReplayPacket();
  ParserState state{};
  net::CNetwork net{&state};
  //std::exit(0);
  bool end = false;
  int AircraftCount = 0;
  while (!end && rdr->getNextPacket(pkt))
  {
    switch(pkt->type)
    {
      case ReplayPacketType::EndMarker:
      {
        LOG("Replay Ending at time {}", ((float)pkt->timestamp_ms)/1000);
        end = true;
        break;
      }
      case ReplayPacketType::StartMarker:
      {
        LOGD("Replay StartMarker\n");
        break;
      }
      case ReplayPacketType::AircraftSmall:
      {
        AircraftCount++;
        break;
      }
      case ReplayPacketType::Chat:
        break;
      case ReplayPacketType::MPI: {
        //LOG("Current Time: %f\n", ((float)pkt->timestamp_ms)/1000);
        auto m = mpi::dispatch(pkt->stream, &state, false);
        if(m != nullptr)
        {
          mpi::send(m);
          delete m;
        }
        break;
      }
      case ReplayPacketType::NextSegment: {
        LOG("NEXTSEGMENT\n");
        break;
      }
      case ReplayPacketType::ECS:
      {
        net.onPacket(pkt, pkt->timestamp_ms);
        break;
      }
      case ReplayPacketType::Snapshot: // useless
        break;
      case ReplayPacketType::ReplayHeaderInfo:
        break;
    }
    //std::cout.flush();
  }
  //auto ptr = &mpi::players;
  for(auto &data : mpi::mpi_data)
  {
    LOG("OID: {:#x}", data.first);
    for(auto &data2: data.second)
    {
      LOG("    mid: {:#x}; count: {}", data2.first, data2.second);
    }
  }
  LOG("Aircraft Count: {}", AircraftCount);
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //rpl.FooterBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->debugPrintEntities();
  //rpl.HeaderBlk.printBlock(0, std::cout);
  //ecs::g_entity_mgr->getTemplateDB()->DebugPrint();
  delete rdr;
  delete srv_rpl;
  delete rpl;
  delete pkt;
  return 0;
}