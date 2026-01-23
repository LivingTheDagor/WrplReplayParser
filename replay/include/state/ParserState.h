#ifndef MYEXTENSION_PARSERSTATE_H
#define MYEXTENSION_PARSERSTATE_H
#include "ecs/EntityManager.h"
#include "mpi/mpi.h"
#include "mpi/codegen/ReflIncludes.h"
#include "network/CNetwork.h"
#include "mpi/ObjectDispatcher.h"

struct ParserState {

  ParserState(int player_count=32) : players(player_count) {}
  ecs::EntityManager g_entity_mgr{};
  std::vector<MPlayer> players;
  std::array<TeamData, 3> teams; // team[0] is global data, teams[1] is first team, teams[2] is second team
  net::CNetwork conn{this};
  mpi::MainDispatch main_dispatch{this};
  void setPlayerCount(int player_count) {
    players.resize(player_count);
  }
};



#endif //MYEXTENSION_PARSERSTATE_H
