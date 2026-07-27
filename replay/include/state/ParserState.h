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
#include "StateRewinder.h"
#include "memory/dag_memBase.h"
#include "StateRewinder.h"

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



struct RewindRef {
  IObjectRewindState *state; // this assumes the memory location of ANY rewindable object will never be changed after registration
  // only for sanity checking
#if LDAG_DBGLEVEL > 0
  uint32_t forward_index;
  uint32_t rewind_index;
#endif
};

// used to hold the state currently being destroyed
// unless if you somehow destroy multiple states at the same time in the same thread
// this will always represent the state currently in destruction, and can be used to pass it around without complex destruction logic
inline thread_local ParserState * _in_destruction_state{};

struct ParserState {

  explicit ParserState(uint32_t player_count = 32);
  explicit ParserState(IReplay *replay);
protected:
  void initialize(uint32_t player_couunt);
  bool is_dtor{false};
  StateAllocator allocator{};
  mpi::MpiQueueObject mpi_queue{this};
  friend mpi::MpiQueueObject;

  friend mpi::IObject *mpi::ObjectDispatcher(mpi::ObjectID oid, mpi::ObjectExtUID extUid, ParserState *state);
  friend StateRewinder;


  // could replace with a tuple vector, basically the same thing with less space
  std::pmr::vector<ecs::EntityId> uid_lookup{&allocator};
  std::pmr::vector<unit::Unit *> uid_unit_lookup{&allocator};

  // when we started collecting states
  uint32_t state_update_start_time_ms{};
  // holds all the current updates for the current time_ms
  std::pmr::vector<RewindRef> curr_ms_rewind_refs{&allocator};

#if LDAG_DBGLEVEL > 0
  void registerStateChange(IObjectRewindState *state, uint32_t back, uint32_t curr);
#else
  void registerStateChange(IObjectRewindState *state);
#endif

  void beforePacket(ReplayPacket &pkt);

  friend IObjectRewindState;

public:

  template <class T, class... Args>
  T * _new(Args&&... args) {
    return allocator._new<T>(std::forward<Args>(args)...);
  }
  template <class T>
  void _delete(T *ptr) {
    allocator._delete(ptr);
  }
  StateAllocator * get_allocator() { return &allocator; }

  StateAllocator::DagAllocType getMem() { return allocator.getMem(); }
  std::vector<mpi::MpiQueueObject::QueueData> *get_queued_data(ecs::EntityId eid) {
    auto it = mpi_queue.dispatched_objects.find(eid);
    if (it == mpi_queue.dispatched_objects.end())
      return nullptr;
    return &it->second;
  }

  uint32_t replay_length_ms = 0xFFFFFFFF;
  uint32_t curr_time_ms = 0; // the current time in the ECS / state.
  net::CNetwork conn{this};
  mpi::GeneralObject main_dispatch{this};
  net_delta_t NetDelta{allocator.getMem()};
  std::pmr::vector<MPlayer> players{get_allocator()};
  ecs::EntityManager g_entity_mgr{this}; // this order is required as g_entity_mgr needs to be destroyed before players
  std::pmr::vector<ObjectRewindState<MissionZone*, false, true>*> Zones{get_allocator()};
  std::array<TeamData, 3> teams{
    TeamData{this, (0xf<<0xb)+0},
    TeamData(this, (0xf<<0xb)+1),
    TeamData(this, (0xf<<0xb)+2)
  }; // team[0] is global data, teams[1] is first team, teams[2] is second team
  std::pmr::vector<ChatMessage> chatMessages{get_allocator()};
  GlobalElo glob_elo{this};
  GeneralState gen_state{this};
  std::pmr::vector<const mpi::IBattleMessage *> BattleMessages{get_allocator()};
  // missionArea1 owns the ptrs
  std::pmr::vector<ObjectRewindState<MissionArea*, false, true>*> missionAreas1{get_allocator()};
  std::pmr::vector<ObjectRewindState<MissionArea*, false>*> missionAreas2{get_allocator()};

  int current_packet_index = -1;


  ecs::EntityId getUnitEid(uint16_t uid) const;

  const std::pmr::vector<unit::Unit*> &getUnitLookup() const {
    return this->uid_unit_lookup;
  }

  unit::Unit *getUnitObj(uint16_t uid) const;

  void setUnitData(uint16_t uid, unit::Unit *unit, ecs::EntityId eid);

  ~ParserState();

  bool ParsePacket(ReplayPacket &pkt);

  void rewindToMs(uint32_t time_ms);

  bool finishedLoading() const { return this->replay_length_ms != 0xFFFFFFFF; }

private:
  StateRewinder rewinder{this};
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
template<typename T, bool do_compare, bool take_ownership, bool create_default>
ObjectRewindState<T, do_compare, take_ownership, create_default>::~ObjectRewindState() {
  if constexpr (take_ownership) {
    auto g_state = _in_destruction_state;
    for (auto &s: this->time_states) {
      g_state->_delete(s.data);
    }
  }
}

#if LDAG_DBGLEVEL > 0
template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::rewindForward(uint32_t expected_idx) {
  G_ASSERT(this->curr_index < this->time_states.size()-1);
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
#if LDAG_DBGLEVEL > 0
  DG_ASSERT(!hasReserved);
  hasReserved = true;
#endif
  auto back = &this->time_states.push_back();
  state = back;
  return &back->data;
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::deleteLast() {
#if LDAG_DBGLEVEL > 0
  G_ASSERT(hasReserved);
  hasReserved = false;
#endif
  this->time_states.pop_back();
  state = &this->time_states.back();
}

template<typename T, bool do_compare, bool take_ownership, bool create_default>
void ObjectRewindState<T, do_compare, take_ownership, create_default>::checkAndPush(ParserState *state) {
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
  this->time_states.back().time_ms = state->curr_time_ms;
  this->state = &this->time_states.back();
  curr_index = this->time_states.size()-1;
  if constexpr (!create_default) {
    if (curr_index == 0)
      return;
  }
#if LDAG_DBGLEVEL > 0
  // we always create one component in ctor, so there will always be at least '2' components when serializing state change
  this->pushBackState(state, this->curr_index-1, this->curr_index);
#else
  this->pushBackState(state);
#endif
}

#endif // MYEXTENSION_PARSERSTATE_H
