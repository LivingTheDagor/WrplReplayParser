#include "state/ParserState.h"
#include "ecs/ComponentTypesDefs.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"

static void on_rocket_appear_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid,
                                Rocket &rocket_component, ecs::EntityManager &manager) {
  rocket_component.type = StoreType::Rocket;
  rocket_component.created_at_ms = *manager.curr_time_ms;
  rocket_component.eid = eid;
}

static void on_rocket_disappear_es(const ecs::EventEntityDestroyedBasic &evt, Rocket &rocket_component,
                                   ecs::EntityManager &manager) {
  if (evt.get<1>())
    rocket_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_bomb_appear_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid, Bomb &bomb_component,
                              ecs::EntityManager &manager) {
  bomb_component.type = StoreType::Bomb;
  bomb_component.created_at_ms = *manager.curr_time_ms;
  bomb_component.eid = eid;
}

static void on_bomb_disappear_es(const ecs::EventEntityDestroyedBasic &evt, Bomb &bomb_component,
                                 ecs::EntityManager &manager) {
  if (evt.get<1>())
    bomb_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_torpedo_appear_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid,
                                 Torpedo &torpedo_component, ecs::EntityManager &manager) {
  torpedo_component.type = StoreType::Torpedo;
  torpedo_component.created_at_ms = *manager.curr_time_ms;
  torpedo_component.eid = eid;
}

static void on_torpedo_disappear_es(const ecs::EventEntityDestroyedBasic &evt, Torpedo &torpedo_component,
                                    ecs::EntityManager &manager) {
  if (evt.get<1>())
    torpedo_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_payload_appear_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid,
                                 Payload &payload_component, ecs::EntityManager &manager) {
  payload_component.type = StoreType::Payload;
  payload_component.created_at_ms = *manager.curr_time_ms;
  payload_component.eid = eid;
}

static void on_payload_disappear_es(const ecs::EventEntityDestroyedBasic &evt, Payload &payload_component,
                                    ecs::EntityManager &manager) {
  if (evt.get<1>())
    payload_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_jettisoned_appear_es(const ecs::EventEntityCreated &evt, const ecs::EntityId eid,
                                    Jettisoned &jettisoned_component, ecs::EntityManager &manager) {
  jettisoned_component.type = StoreType::Jettisoned;
  jettisoned_component.created_at_ms = *manager.curr_time_ms;
  jettisoned_component.eid = eid;
}

static void on_jettisoned_disappear_es(const ecs::EventEntityDestroyedBasic &evt, Jettisoned &jettisoned_component,
                                       ecs::EntityManager &manager) {
  if (evt.get<1>())
    jettisoned_component.destroyed_at_ms = *manager.curr_time_ms;
}

template<typename Callable>
static void iterate_all_rockets_ecs_query(ecs::EntityManager &manager, Callable c);

namespace unit {
  void buildBallisticArc(Rocket &store, bool powered, float sea_level);
}

// The collectors below return every store the battle created, tracked or not.
// Unguided ordnance gets no positions: the server does not stream a free fall, the
// client integrates it. Such a store is an entity with a release state, a drop time
// and a death time and nothing in between, which is enough to rebuild the arc, so
// dropping it here would throw away the only record of it.
std::vector<Rocket *> collect_all_rockets(ParserState &state) {
  std::vector<Rocket *> rockets;
  iterate_all_rockets_ecs_query(state.g_entity_mgr, [&rockets, sea = state.sea_level](Rocket &rocket_component) {
    unit::buildBallisticArc(rocket_component, /*powered*/ true, sea);
    rockets.push_back(&rocket_component);
  });
  return rockets;
}

template<typename Callable>
static void iterate_all_torpedoes_ecs_query(ecs::EntityManager &manager, Callable c);

// No arc for a torpedo: the ballistic model integrates a body in air, and this one runs
// in water.
std::vector<Torpedo *> collect_all_torpedoes(ParserState &state) {
  std::vector<Torpedo *> torpedoes;
  iterate_all_torpedoes_ecs_query(state.g_entity_mgr, [&torpedoes](Torpedo &torpedo_component) {
    torpedoes.push_back(&torpedo_component);
  });
  return torpedoes;
}

template<typename Callable>
static void iterate_all_payloads_ecs_query(ecs::EntityManager &manager, Callable c);

// Payload and jettisoned stores were collected by nobody, so whatever the game put
// under those components never reached the caller even when its positions arrived.
std::vector<Payload *> collect_all_payloads(ParserState &state) {
  std::vector<Payload *> payloads;
  // No arc for a payload: it hangs on the pylon from spawn, so integrating from its
  // creation to its removal would draw a battle long free fall from the spawn point.
  // A tank that is actually let go becomes a Jettisoned store and gets its arc there.
  iterate_all_payloads_ecs_query(state.g_entity_mgr, [&payloads](Payload &payload_component) {
    payloads.push_back(&payload_component);
  });
  return payloads;
}

template<typename Callable>
static void iterate_all_jettisoned_ecs_query(ecs::EntityManager &manager, Callable c);

std::vector<Jettisoned *> collect_all_jettisoned(ParserState &state) {
  std::vector<Jettisoned *> jettisoned;
  iterate_all_jettisoned_ecs_query(state.g_entity_mgr,
                                   [&jettisoned, sea = state.sea_level](Jettisoned &jettisoned_component) {
    unit::buildBallisticArc(jettisoned_component, /*powered*/ false, sea);
    jettisoned.push_back(&jettisoned_component);
  });
  return jettisoned;
}

template<typename Callable>
static void iterate_all_bombs_ecs_query(ecs::EntityManager &manager, Callable c);

std::vector<Bomb *> collect_all_bombs(ParserState &state) {
  std::vector<Bomb *> bombs;
  iterate_all_bombs_ecs_query(state.g_entity_mgr, [&bombs, sea = state.sea_level](Bomb &bomb_component) {
    unit::buildBallisticArc(bomb_component, /*powered*/ true, sea);
    bombs.push_back(&bomb_component);
  });
  return bombs;
}
