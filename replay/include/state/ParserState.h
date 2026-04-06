#ifndef MYEXTENSION_PARSERSTATE_H
#define MYEXTENSION_PARSERSTATE_H
#include "ecs/EntityManager.h"
#include "mpi/mpi.h"
#include "mpi/codegen/ReflIncludes.h"
#include "network/CNetwork.h"
#include "mpi/ObjectDispatcher.h"
#include "mpi/GeneralObject.h"
#include "tracy/Tracy.hpp"
#include "Replay/Replay.h"

struct ParserState {

  explicit ParserState(int player_count=32) : players(player_count) {}
  explicit ParserState(Replay *replay): players(replay->PlayerCount) {}
  explicit ParserState(ServerReplay *replay): players(replay->replay_files[0].PlayerCount) {}
  explicit ParserState(MemoryEfficientServerReplay *replay): players(replay->base_replay->PlayerCount) {}
  ecs::EntityManager g_entity_mgr{};
  std::vector<MPlayer> players;
  std::vector<BaseZone*> Zones;
  std::array<TeamData, 3> teams; // team[0] is global data, teams[1] is first team, teams[2] is second team
  GlobalElo glob_elo;
  GeneralState gen_state;
  net::CNetwork conn{this};
  mpi::GeneralObject main_dispatch{this};
  BattleMessageHandler battles_messages{this};
  uint32_t curr_time_ms = 0;
  void setPlayerCount(int player_count) {
    players.resize(player_count);
  }
  ~ParserState() {
    ZoneScoped;
    for(auto v : Zones) {
      delete v;
    }
  }
  void onPacket(ReplayPacket *pkt) {
    conn.onPacket(pkt, pkt->timestamp_ms);
  }
};



#endif //MYEXTENSION_PARSERSTATE_H
