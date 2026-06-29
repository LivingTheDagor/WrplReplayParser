#include "state/ParserState.h"
#include "ecs/ComponentTypesDefs.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"

static void on_rocket_appear_es(const ecs::EventEntityCreated &evt, Rocket &rocket_component,
                                ecs::EntityManager &manager) {
  rocket_component.created_at_ms = *manager.curr_time_ms;
}

static void on_rocket_disappear_es(const ecs::EventEntityDestroyed &evt, Rocket &rocket_component,
                                   ecs::EntityManager &manager) {
  rocket_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_bomb_appear_es(const ecs::EventEntityCreated &evt, Bomb &bomb_component, ecs::EntityManager &manager) {
  bomb_component.created_at_ms = *manager.curr_time_ms;
}

static void on_bomb_disappear_es(const ecs::EventEntityDestroyed &evt, Bomb &bomb_component,
                                 ecs::EntityManager &manager) {
  bomb_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_torpedo_appear_es(const ecs::EventEntityCreated &evt, Torpedo &torpedo_component,
                                 ecs::EntityManager &manager) {
  torpedo_component.created_at_ms = *manager.curr_time_ms;
}

static void on_torpedo_disappear_es(const ecs::EventEntityDestroyed &evt, Torpedo &torpedo_component,
                                    ecs::EntityManager &manager) {
  torpedo_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_payload_appear_es(const ecs::EventEntityCreated &evt, Payload &payload_component,
                                 ecs::EntityManager &manager) {
  payload_component.created_at_ms = *manager.curr_time_ms;
}

static void on_payload_disappear_es(const ecs::EventEntityDestroyed &evt, Payload &payload_component,
                                    ecs::EntityManager &manager) {
  payload_component.destroyed_at_ms = *manager.curr_time_ms;
}

static void on_jettisoned_appear_es(const ecs::EventEntityCreated &evt, Jettisoned &jettisoned_component,
                                    ecs::EntityManager &manager) {
  jettisoned_component.created_at_ms = *manager.curr_time_ms;
}

static void on_jettisoned_disappear_es(const ecs::EventEntityDestroyed &evt, Jettisoned &jettisoned_component,
                                       ecs::EntityManager &manager) {
  jettisoned_component.destroyed_at_ms = *manager.curr_time_ms;
}
