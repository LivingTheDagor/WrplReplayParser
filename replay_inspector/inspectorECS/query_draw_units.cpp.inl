#include "state/ParserState.h"
#include "ecs/ComponentTypesDefs.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"
#include "../include/repl/callback.h"
#ifndef _ECS_CODEGEN
#include "imgui.h"
#endif
#include "tracy/Tracy.hpp"

template<typename Callable>
ECS_REQUIRE(ecs::Tag playerUnit, ecs::Tag unit_tag__tank)
static void iterate_all_tanks_ecs_query(ecs::EntityManager &manager, Callable c);

template<typename Callable>
ECS_REQUIRE(ecs::Tag playerUnit, ecs::Tag unit_tag__aircraft)
static void iterate_all_aircraft_ecs_query(ecs::EntityManager &manager, Callable c);

void iterate_all_tanks(ParserState &state, on_unit_cb_t &on_unit) {
  ZoneScoped;
  iterate_all_tanks_ecs_query(state.g_entity_mgr,
    [&on_unit](const unit::UnitRef &unit__ref, const int unit__playerId, ecs::EntityManager &manager) {
      if (unit__playerId != -1 && unit__ref.unit && !unit__ref.unit->positions.history().empty()) {
        auto unit = unit__ref.unit;
        if (unit->destroyed_at_ms >= *manager.curr_time_ms && *manager.curr_time_ms >= unit->created_at_ms) {
          on_unit(*unit);
        }
      }
    });
}

void iterate_all_aircraft(ParserState &state, on_unit_cb_t &on_unit) {
  ZoneScoped;
  iterate_all_aircraft_ecs_query(state.g_entity_mgr,
    [&on_unit](const unit::UnitRef &unit__ref, const int unit__playerId, ecs::EntityManager &manager) {
      if (unit__playerId != -1 && unit__ref.unit && !unit__ref.unit->positions.history().empty()) {
        auto unit = unit__ref.unit;
        if (unit->destroyed_at_ms > *manager.curr_time_ms && *manager.curr_time_ms >= unit->created_at_ms) {
          on_unit(*unit);
        }
      }
    });
}

const char *get_unit_name_1(const unit::Unit &unit) {
  auto idx = translate::localize_index(unit.name_index_1);
  if (idx == nullptr || idx == "") {
    LOGE("WARNING, unit: {} doesnt have index_1", unit.unit_name);
    return unit.unit_name.c_str();
  }
  return idx;
}

template<typename Callable>
ECS_REQUIRE(ecs::Tag playerUnit)
static void iterate_all_player_units_ecs_query(ecs::EntityManager &manager, Callable c);

ImVec2 get_longest_unit_name(ParserState &state) {
  ImVec2 ret{};
  iterate_all_player_units_ecs_query(state.g_entity_mgr, [&ret](const unit::UnitRef &unit__ref) {
    if (unit__ref.unit) {
      auto unit_name = get_unit_name_1(*unit__ref.unit);
      if (unit_name == nullptr || unit_name == "")
        LOGE("WARNING, unit: {} doesnt have index_1", unit__ref.unit->unit_name);
      else {
        ImVec2 temp = ImGui::CalcTextSize(unit_name);
        ret = temp.x > ret.x ? temp : ret;
      }
    }
  });

  return ret;
}


template<typename Callable>
static void iterate_all_rockets_ecs_query(ecs::EntityManager &manager, Callable c);


void iterate_all_rockets(ParserState &state, on_rocket_cb_t &on_weapon) {
  ZoneScoped iterate_all_rockets_ecs_query(state.g_entity_mgr,
                                           [&on_weapon](const Rocket &rocket_component, ecs::EntityManager &manager) {
                                             if (!rocket_component.positions.empty()) {
                                               if (rocket_component.destroyed_at_ms > *manager.curr_time_ms &&
                                                   *manager.curr_time_ms >= rocket_component.created_at_ms) {
                                                 on_weapon(rocket_component);
                                               }
                                             }
                                           });
}


template<typename Callable>
static void iterate_all_bombs_ecs_query(ecs::EntityManager &manager, Callable c);

void iterate_all_bombs(ParserState &state, on_bomb_cb_t &on_weapon) {
  ZoneScoped iterate_all_bombs_ecs_query(state.g_entity_mgr,
                                         [&on_weapon](const Bomb &bomb_component, ecs::EntityManager &manager) {
     if (!bomb_component.positions.empty()) {
       if (bomb_component.destroyed_at_ms > *manager.curr_time_ms &&
           *manager.curr_time_ms >= bomb_component.created_at_ms) {
         on_weapon(bomb_component);
       }
     }
   });
}