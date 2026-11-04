#pragma once
#include "ecs/EntityManager.h"
#include "mpi/mpi.h"
#include "mpi/codegen/ReflIncludes.h"
#include "network/CNetwork.h"
#include "mpi/ObjectDispatcher.h"
#include "mpi/GeneralObject.h"
#include "Replay/Replay.h"
#include "daNet/delta/deltaCompression.h"
#ifndef _ECS_CODEGEN
#include "tracy/Tracy.hpp"
#endif
#include "StateAllocator.h"
#include "StateRewinder.h"
#include "memory/dag_memBase.h"
#include "mpi/Ballistics.h"
#include "blockAllocator.h"
#include "chat.h"
#include "mpi/PositionSync.h"
#include "internal/net_delta.h"

class PyReplayState;
namespace unit {
  class Unit;
}
namespace parserInt {
  class ParserShare;
}
namespace pybind11 {
  template<typename type_, typename... options>
  class class_;
}


// used to hold the state currently being destroyed
// unless if you somehow destroy multiple states at the same time in the same thread
// this will always represent the state currently in destruction, and can be used to pass it around without complex
// destruction logic
inline thread_local ParserState *_in_destruction_state{};

struct ParserState {

  explicit ParserState(uint32_t player_count = 32);
  explicit ParserState(replay::IReplay *replay);

protected:
  void initialize(uint32_t player_count);
  template<typename... Ts>
  struct UnitAllocator {
  private:
    using AllocT = StateAllocator::DagAllocType;
    static_assert((std::is_base_of_v<unit::Unit, Ts> && ...), "must derive from unit::Unit");
    std::tuple<BlockAllocator<Ts, 16, AllocT>...> containers;

  public:
    explicit UnitAllocator(AllocT alloc_type) : containers{((void) sizeof(Ts), alloc_type)...} {}

    template<typename T, typename... Args>
    T &allocateOne(Args &&...args) {
      return std::get<BlockAllocator<T, 16, AllocT>>(containers).getNext(std::forward<Args>(args)...);
    }
  };

#if LDAG_DBGLEVEL > 0
  void registerStateChange(IObjectRewindState *state, uint32_t back, uint32_t curr);
#else
  void registerStateChange(IObjectRewindState *state);
#endif

  void beforePacket(ReplayPacket &pkt);

public:
  struct RewindRef {
    IObjectRewindState
      *state; // this assumes the memory location of ANY rewindable object will never be changed after registration
    // only for sanity checking
#if LDAG_DBGLEVEL > 0
    uint32_t forward_index;
    uint32_t rewind_index;
#endif
  };
  template<class T, class... Args>
  T *_new(Args &&...args) {
    return allocator._new<T>(std::forward<Args>(args)...);
  }
  template<class T>
  void _delete(T *ptr) {
    allocator._delete(ptr);
  }
  StateAllocator *get_allocator() { return &allocator; }

  StateAllocator::DagAllocType getMem() { return allocator.getMem(); }
  std::vector<mpi::MpiQueueObject::QueueData> *get_queued_data(ecs::EntityId eid) {
    auto it = mpi_queue.dispatched_objects.find(eid);
    if (it == mpi_queue.dispatched_objects.end())
      return nullptr;
    return &it->second;
  }

  ecs::EntityId getUnitEid(uint16_t uid) const;

  const std::pmr::vector<unit::Unit *> &getUnitLookup() const { return this->uid_unit_lookup; }

  unit::Unit *getUnitObj(uint16_t uid) const;

  void setUnitData(uint16_t uid, unit::Unit *unit, ecs::EntityId eid);

  void recordTeamSpawnSide(int team, const std::string &mission_name);

  ~ParserState();

  bool ParsePacket(ReplayPacket &pkt);

  void rewindToMs(uint32_t time_ms);

  bool finishedLoading() const { return this->replay_length_ms != 0xFFFFFFFF; }

  uint32_t getReplayLengthMs() const { return this->replay_length_ms; }
  uint32_t getCurrTimeMs() const { return this->curr_time_ms; }
  float getSeaLevel() const { return this->sea_level; }
  const auto &getPlayers() const { return this->players; }
  const auto &getEntityManager() const { return this->g_entity_mgr; }
  const auto &getZones() const { return this->Zones; }
  const auto &getTeams() const { return this->teams; }
  const auto &getChatMessages() const { return this->chatMessages; }
  const auto &getGlobalElo() const { return this->glob_elo; }
  const auto &getGeneralState() const { return this->gen_state; }
  const auto &getBattleMessages() const { return this->BattleMessages; }
  const auto &getHitAffects() const { return this->HitEffects; }
  const auto &getHitAnalysis() const { return this->HitAnalyses; }
  const auto &getHitDamages() const { return this->HitDamages; }
  const auto &getHitDirections() const { return this->HitDirections; }
  const auto &getHitExplosions() const { return this->HitExplosions; }
  const auto &getHitOutcomes() const { return this->HitOutcomes; }
  const auto &getAmmoEvents() const { return this->AmmoEvents; }
  const auto &getShotEvents() const { return this->ShotEvents; }
  const auto &getMissionAreas1() const { return this->missionAreas1; }
  const auto &getMissionAreas2() const { return this->missionAreas2; }
  const auto &getCurrentPacketIndex() const { return this->current_packet_index; }

protected:
  bool is_dtor{false};
  StateAllocator allocator{};
  mpi::MpiQueueObject mpi_queue{this};


  // could replace with a tuple vector, basically the same thing with less space
  std::pmr::vector<ecs::EntityId> uid_lookup{&allocator};
  std::pmr::vector<unit::Unit *> uid_unit_lookup{&allocator};

  // when we started collecting states
  uint32_t state_update_start_time_ms{};
  // holds all the current updates for the current time_ms
  std::pmr::vector<RewindRef> curr_ms_rewind_refs{&allocator};
  // used by ballisitics loader as cache
  std::unordered_map<std::string, unit::BallisticParams> ballistics_cache;
  std::unordered_map<std::string, std::vector<unit::BulletSet>> bullet_set_cache;
  UnitAllocator<unit::Aircraft, unit::Tank> _unit_allocator{allocator.getMem()};

  uint32_t replay_length_ms = 0xFFFFFFFF;
  uint32_t curr_time_ms = 0; // the current time in the ECS / state.
  /// Sea level of the map, metres, out of levels/<stem>.blk. Altitude is packed
  /// relative to it, so a y of the world is this much above the sea. Zero when the
  /// level file does not say, which is what most maps do.
  float sea_level = 0.f;
  net::CNetwork conn{this};
  mpi::GeneralObject main_dispatch{this};
  net_delta_t NetDelta{allocator.getMem()};
  std::pmr::vector<MPlayer> players{get_allocator()};
  ecs::EntityManager g_entity_mgr{this}; // this order is required as g_entity_mgr needs to be destroyed before players
  std::pmr::vector<ObjectRewindState<MissionZone *, false, true> *> Zones{get_allocator()};
  std::array<TeamData, 3> teams{
    TeamData{this, (0xf << 0xb) + 0}, TeamData(this, (0xf << 0xb) + 1),
    TeamData(this, (0xf << 0xb) + 2)}; // team[0] is global data, teams[1] is first team, teams[2] is second team
  std::pmr::vector<ChatMessage> chatMessages{get_allocator()};
  GlobalElo glob_elo{this};
  GeneralState gen_state{this};
  std::pmr::vector<const mpi::IBattleMessage *> BattleMessages{get_allocator()};
  // Shot and hit packets, one list per message and nothing joined: a record here
  // is what the packet said. See mpi/GeneralObject.h for what joins to what.
  std::pmr::vector<mpi::HitEffect> HitEffects{get_allocator()};
  std::pmr::vector<mpi::HitAnalysis> HitAnalyses{get_allocator()};
  std::pmr::vector<mpi::HitDamage> HitDamages{get_allocator()};
  std::pmr::vector<mpi::HitDirection> HitDirections{get_allocator()};
  std::pmr::vector<mpi::HitExplosion> HitExplosions{get_allocator()};
  std::pmr::vector<mpi::HitOutcome> HitOutcomes{get_allocator()};
  std::pmr::vector<mpi::AmmoEvent> AmmoEvents{get_allocator()};
  std::pmr::vector<mpi::ShotEvent> ShotEvents{get_allocator()};
  // missionArea1 owns the ptrs
  std::pmr::vector<ObjectRewindState<MissionArea *, false, true> *> missionAreas1{get_allocator()};
  std::pmr::vector<ObjectRewindState<MissionArea *, false> *> missionAreas2{get_allocator()};

  int current_packet_index = -1;
  StateRewinder rewinder{this};
  std::array<uint8_t, 3> team_spawn_side_masks{};
  friend StateRewinder;
  friend unit::BallisticParams;
  friend unit::BulletSet;
  friend IObjectRewindState;
  friend parserInt::ParserShare;
  friend mpi::MpiQueueObject;
  friend dataSync;
  friend BaseExtReflectable;
  friend MissionZone *create_zone(danet::BitStream &bs, uint8_t zone_type, ParserState *state);
  friend MissionZone;
  friend MissionArea;
  friend net::CNetwork;
  friend ecs::EntityManager;
  friend ecs::EcsRewindEvent;
  template<typename type_, typename... options>
  friend class pybind11::class_;
  friend PyReplayState;
};

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void *ObjectRewindState<T, do_compare, take_ownership, create_default>::reserveOneV() {
  return this->reserveOne();
}
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::deleteLastV() {
  this->deleteLast();
}
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::checkAndPushV(ParserState *state) {
  this->checkAndPush(state);
}
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void *ObjectRewindState<T, do_compare, take_ownership, create_default>::getPtr() {
  return &this->state->data;
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
ObjectRewindState<T, do_compare, take_ownership, create_default>::ObjectRewindState() : time_states() {
  if constexpr (create_default) {
    this->time_states.emplace_back(TimeState{});
    this->time_states.back().time_ms = 0;
    this->state = &this->time_states.back();
  } else {
    this->state = nullptr;
  }
}


#if LDAG_DBGLEVEL > 0
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::rewindForward(uint32_t expected_idx) {
  G_ASSERT(this->curr_index < this->time_states.size() - 1);
  this->curr_index++;
  ++this->state;
  G_ASSERT(this->curr_index == expected_idx);
  G_ASSERT(this->state == &this->time_states[this->curr_index]);
}
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::rewindBackward(uint32_t expected_idx) {
  G_ASSERT(this->curr_index > 0);
  this->curr_index--;
  --this->state;
  G_ASSERT(this->curr_index == expected_idx);
  G_ASSERT(this->state == &this->time_states[this->curr_index]);
}

#else
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::rewindForward() {
  this->curr_index++;
  ++this->state;
}
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::rewindBackward() {
  this->curr_index--;
  --this->state;
}
#endif

template<typename T, bool do_compare, bool take_ownership, bool create_default>
T *ObjectRewindState<T, do_compare, take_ownership, create_default>::reserveOne() {
  ZoneScopedN("ObjectRewindState::reserveOne");
#if LDAG_DBGLEVEL > 0
  G_ASSERT(!hasReserved);
  hasReserved = true;
#endif
  auto back = &this->time_states.push_back();
  state = back;
  return &back->data;
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::pushAt(uint32_t time_ms, const T &value) {
  // Ownership is not handled here: an owning history would need the pointer registered
  // for release, and this path deliberately skips the rewind bookkeeping.
  static_assert(!take_ownership, "pushAt cannot be used on an owning history");
  auto &back = this->time_states.push_back();
  back.time_ms = time_ms;
  back.data = value;
  this->state = &this->time_states.back();
  this->curr_index = this->time_states.size() - 1;
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::deleteLast() {
#if LDAG_DBGLEVEL > 0
  G_ASSERT(hasReserved);
  hasReserved = false;
#endif
  this->time_states.pop_back();
  state = this->time_states.empty() ? nullptr : &this->time_states.back();
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::checkAndPush(ParserState *state) {
  ZoneScopedN("ObjectRewindState::checkAndPush");
#if LDAG_DBGLEVEL > 0
  G_ASSERT(hasReserved);
  hasReserved = false;
#endif
  if constexpr (do_compare) {
    if (this->time_states.size() > 1) {
      auto &prev = this->time_states[this->time_states.size() - 2];
      auto &curr = this->time_states.back();
      if (prev.data == curr.data) {
        this->time_states.pop_back();
        this->state = &this->time_states.back();
        return;
      }
    }
  }
  this->time_states.back().time_ms = state->getCurrTimeMs();
  this->state = &this->time_states.back();
  curr_index = this->time_states.size() - 1;
  if constexpr (!create_default) {
    if (curr_index == 0)
      return;
  }
#if LDAG_DBGLEVEL > 0
  // we always create one component in ctor, so there will always be at least '2' components when serializing state
  // change
  this->pushBackState(state, this->curr_index - 1, this->curr_index);
#else
  this->pushBackState(state);
#endif
}
