#ifndef MYEXTENSION_PARSERSTATE_H
#define MYEXTENSION_PARSERSTATE_H
#include "ecs/EntityManager.h"
#include "mpi/mpi.h"
#include "mpi/codegen/ReflIncludes.h"
#include "network/CNetwork.h"
#include "mpi/ObjectDispatcher.h"
#include "mpi/GeneralObject.h"
#include "Replay/Replay.h"
#include "danet/delta/deltaCompression.h"
#ifndef _ECS_CODEGEN
#include "tracy/Tracy.hpp"
#endif
#include "StateAllocator.h"
#include "memory/dag_memBase.h"

namespace unit {
  class Unit;
}

struct net_delta_t {
  net_delta_t(IMemAlloc *alloc) : allocator(alloc), netDelta(5, 0xd, alloc) {}
  IMemAlloc *allocator;
  net::DeltaComp netDelta{5, 0xd, allocator};
  std::vector<net::DeltaComp::History> histories{};
  net::DeltaComp::History &getHistory(uint16_t uid) {
    if (uid >= histories.size()) {
      histories.resize(uid + 1);
    }
    auto &hist = histories[uid];
    if (!hist.isInited()) {
      // use_cache only ever matters in a network context
      netDelta.initHistory(hist, true, false);
    }
    return hist;
  }
};

enum ChatType : uint8_t {
  Team = 0,
  All = 1,
  Squad = 2,
  Direct = 3, // exists, but is currently unused from what I can tell
};

struct ChatMessage {
  uint32_t time_ms;
  std::string player_name; // player name
  std::string message; // message text
  ChatType channel = All; // what channel this message was sent on
  bool is_local_message{}; // a message that only ur client sees (ex: chat spamming message)
  bool is_quick_message{}; // is a message created through the quick message menu
  std::string complaints; // this is basically useless information
  inline bool FromBS(BitStream &bs);
};

struct ParserState {

  explicit ParserState(int player_count = 32) : players(player_count) {}
  explicit ParserState(IReplay *replay) : players(replay->getHeader()->player_count) {}
protected:
  StateAllocator allocator{};
  mpi::MpiQueueObject mpi_queue{};
  friend mpi::MpiQueueObject;

  friend mpi::IObject *mpi::ObjectDispatcher(mpi::ObjectID oid, mpi::ObjectExtUID extUid, ParserState *state);


  // could replace with a tuple vector, basically the same thing with less space
  std::vector<ecs::EntityId> uid_lookup{};
  std::vector<unit::Unit *> uid_unit_lookup{};

  void onPacket(ReplayPacket *pkt) { conn.onPacket(pkt, pkt->timestamp_ms); }

public:
  std::pmr::memory_resource * get_allocator() { return &allocator; }
  IMemAlloc * getMem() { return allocator.getMem(); }
  std::vector<mpi::MpiQueueObject::QueueData> *get_queued_data(ecs::EntityId eid) {
    auto it = mpi_queue.dispatched_objects.find(eid);
    if (it == mpi_queue.dispatched_objects.end())
      return nullptr;
    return &it->second;
  }

  uint32_t replay_length_ms = 0xFFFFFFFF;
  uint32_t current_rewind_ms; // the time we have rewinded to
  uint32_t curr_time_ms = 0; // the current time in the ECS / state. this wont always math current_rewind_time_ms
  net::CNetwork conn{this};
  mpi::GeneralObject main_dispatch{this};
  net_delta_t NetDelta{allocator.getMem()};
  std::pmr::vector<MPlayer> players{get_allocator()};
  ecs::EntityManager g_entity_mgr{this}; // this order is required as g_entity_mgr needs to be destroyed before players
  std::pmr::vector<MissionZone *> Zones{get_allocator()};
  std::array<TeamData, 3> teams{}; // team[0] is global data, teams[1] is first team, teams[2] is second team
  std::pmr::vector<ChatMessage> chatMessages{get_allocator()};
  GlobalElo glob_elo{};
  GeneralState gen_state{};
  std::pmr::vector<const mpi::IBattleMessage *> BattleMessages{get_allocator()};
  std::pmr::vector<MissionArea *> missionAreas1{get_allocator()};
  std::pmr::vector<MissionArea *> missionAreas2{get_allocator()};

  int current_packet_index = -1;


  ecs::EntityId getUnitEid(uint16_t uid);

  unit::Unit *getUnitObj(uint16_t uid);

  void setUnitData(uint16_t uid, unit::Unit *unit, ecs::EntityId eid);

  void setPlayerCount(int player_count) { players.resize(player_count); }

  ~ParserState();

  bool ParsePacket(ReplayPacket &pkt);

  inline void rewindToMs(uint32_t time_ms);

  bool finishedLoading() { return this->replay_length_ms != 0xFFFFFFFF; }
};


#endif // MYEXTENSION_PARSERSTATE_H
