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

enum DispatcherFlags {
  Dispatch = 1,
  Reflection = 2,
  Both = 3,
};

struct ParserConfig {
  struct {
    /// do we create tank units? As in, when an tank entity arrives, do we create the unit::Tank?
    bool load_tanks = true;
    /// do we parse tank positions?
    /// mpi message 0xf074 / 0xf073
    bool enable_GMSync = true;
    /// do we parse and load turret data?
    bool turret_parsing = true;
  } GMOpts;
  bool tanks_enabled() const { return GMOpts.load_tanks; }
  bool GMSync_enabled() const { return tanks_enabled() && GMOpts.enable_GMSync; }
  bool turret_parsing_enabled() const { return GMOpts.turret_parsing; }

  struct {
    /// do we create aircraft units? As in, when an aircraft entity arrives, do we create the unit::Aircraft?
    bool load_aircraft = true;
    /// do we parse flight model data?
    /// (see ReplayPacketType::AircraftSmall)
    bool enable_FMSync = true;
    /// do we capture the ammo delta from FMSync?
    bool capture_ammo_events = true;
  } FMOpts;

  bool aircraft_enabled() const { return FMOpts.load_aircraft; }
  bool FMSync_enabled() const { return aircraft_enabled() && FMOpts.enable_FMSync; }
  bool capture_ammo_events() const { return FMOpts.capture_ammo_events; }

  struct {
    /// do we parse ecs packets? this included entity construction
    /// (see ReplayPacketType::ECS)
    bool parse_ecs = true;

    /// do we parse ecs messages? (currently not implemented)
    bool parse_ecs_message = true;
    /// TODO: add support for selection of templates
  } ECSOpts;

  bool ecs_enabled() const { return ECSOpts.parse_ecs; }
  bool ecs_messages_enabled() const { return ECSOpts.parse_ecs_message; }

  struct {
    /// do we parse mpi packets? this includes, GMSync, reflection, etc ...
    /// (see ReplayPacketType::MPI)
    bool parse_mpi = true;

    /// do we parse reflection packets? this includes player state, team state, some global data, unit
    /// reflection data, etc ...
    /// mpi type 0xf0aa, 0xf02d, 0xd136
    bool parse_reflection = true;


    /// do we parse mpi replication messages? this includes stuff like capture points, bombing points, airfields, Zones,
    /// etc...
    /// mpi type 0xd039
    bool parse_replication = true;

    /// do we parse kill, severe damage, critical damage,and award messages?
    /// mpi type 0xf058, 0xf157, 0xf056, 0xf078
    bool parse_battle_message = true;

    /// do we parse the weapon syncs of rockets, bombs, and torpedoes?
    /// this also disables weapon prediction calculation that is normally applied for specific weapons that aren't
    /// synced see mpi types 0xf11a, 0xf0db
    bool parse_weapon_sync = true;
  } MpiOpts;

  bool mpi_enabled() const { return MpiOpts.parse_mpi; }
  bool reflection_enabled() const { return MpiOpts.parse_reflection; }
  bool replication_enabled() const { return MpiOpts.parse_replication; }
  bool battle_messages_enabled() const { return MpiOpts.parse_battle_message; }
  bool weapon_sync_enabled() const { return MpiOpts.parse_weapon_sync; }

protected:
  bool use_enabled_oids = false;
  /// array of enabled object IDs see mpi::MpiQueueObject::ObjectDispatcher on how various oids are used
  /// this controls both what objects can be dispatched for base mpi and reflection
  std::array<bool, mpi::ExtMpiTypes::WEAP> enabled_oids = {true};

  bool use_enabled_reflection_oids = false;

  /// oid whitelist, but for reflection dispatch instead
  std::array<bool, mpi::ExtMpiTypes::WEAP> enabled_reflection_oids = {false};

  bool use_disabled_mids = false;
  std::unordered_set<mpi::MessageID> disabled_mids{};

  bool use_enabled_mids = false;
  std::unordered_set<mpi::MessageID> enabled_mids{};

  friend mpi::Message *mpi::dispatch(const danet::BitStream &bs, ParserState *state, bool copy_payload);
  friend int danet::deserializeReflectables(danet::BitStream &bs, mpi::object_dispatcher resolver, ParserState *state);

public:
  /// wrapper around set_enabled_oids
  /// lets you do options for unit dispatch oids
  void set_unit_dispatch_state(bool enabled, DispatcherFlags flags);


  /// this controls the oids that are enabled for dispatch. by default, all oids are enabled
  /// when this is called, oid filtering is enabled. control this with set_oid_enabled_state.
  /// @param to_enabled the list of oids to control
  /// @param enable if the listed oids will be allowed or blocked
  /// @param flags if you want to control oids for dispatch, reflection, or both
  template<size_t T>
  void set_enabled_oids(const std::array<uint8_t, T> &to_enabled, bool enable, DispatcherFlags flags);
  /// enable or disable oid whitelist for dispatch
  void set_oid_enabled_state(bool to_enable, DispatcherFlags flags);

  /// set the mids that should be disabled, a blacklist
  /// also turns on filtering mids by blacklist, call set_mid_disabled_state to change that
  template<size_t T>
  void set_disabled_mids(const std::array<mpi::MessageID, T> &to_disabled);
  /// enable or disable mid blacklist
  void set_mid_disabled_state(bool to_disable);

  /// set the mids that should be enabled, a whitelist
  /// also turns on filtering mids by whitelist, call set_mid_enabled_state to change that
  template<size_t T>
  void set_enabled_mids(const std::array<mpi::MessageID, T> &to_enabled);
  /// enable or disable mid whitelist
  void set_mid_enabled_state(bool to_enable);
  /// do we parse chat packet type? (see ReplayPacketType::Chat)
  bool parse_chat = true;

  bool chat_enabled() const { return parse_chat; }
};

template<size_t T>
void ParserConfig::set_enabled_oids(const std::array<uint8_t, T> &to_enabled, bool enable, DispatcherFlags flags) {
  if (flags & DispatcherFlags::Dispatch)
    use_enabled_oids = true;
  if (flags & DispatcherFlags::Reflection)
    use_enabled_reflection_oids = true;
  for (size_t i = 0; i < T; i++) {
    G_ASSERTF_AND_DO_UNHYGIENIC(to_enabled[i] < enabled_oids.size(), continue;
                                , "invalid mpi object id {} to enable", to_enabled[i]);
    if (flags & DispatcherFlags::Dispatch)
      enabled_oids[to_enabled[i]] = enable;
    if (flags & DispatcherFlags::Reflection)
      enabled_reflection_oids[to_enabled[i]] = enable;
  }
}

template<size_t T>
void ParserConfig::set_disabled_mids(const std::array<mpi::MessageID, T> &to_disabled) {
  this->use_disabled_mids = true;
  for (size_t i = 0; i < T; i++) {
    this->disabled_mids.insert(to_disabled[i]);
  }
}
template<size_t T>
void ParserConfig::set_enabled_mids(const std::array<mpi::MessageID, T> &to_enabled) {
  this->use_enabled_mids = true;
  for (size_t i = 0; i < T; i++) {
    this->enabled_mids.insert(to_enabled[i]);
  }
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
  void setUnitData(uint16_t uid, unit::Unit *unit, ecs::EntityId eid);
  void recordTeamSpawnSide(int team, const std::string &mission_name);
  std::vector<mpi::MpiQueueObject::QueueData> *get_queued_data(ecs::EntityId eid) {
    auto it = mpi_queue.dispatched_objects.find(eid);
    if (it == mpi_queue.dispatched_objects.end())
      return nullptr;
    return &it->second;
  }

public:
  bool ParsePacket(ReplayPacket &pkt);
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

  ecs::EntityId getUnitEid(uint16_t uid) const;

  const std::pmr::vector<unit::Unit *> &getUnitLookup() const { return this->uid_unit_lookup; }

  unit::Unit *getUnitObj(uint16_t uid) const;

  ~ParserState();

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
  auto &getConfig() { return this->config; }

protected:
  bool is_dtor{false};
  StateAllocator allocator{};
  mpi::MpiQueueObject mpi_queue{this};
  ParserConfig config{};


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
