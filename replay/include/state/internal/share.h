#pragma once
#include "state/ParserState.h"
#define SHARE_STRUCT(name)             _hello_hello_name_manglanagzer_##name
#define DECL_STRUCT_SHARE(name)        struct SHARE_STRUCT(name);
#define DECL_STRUCT_SHARE_FRIEND(name) friend SHARE_STRUCT(name);

DECL_STRUCT_SHARE(BaseEntityES)
DECL_STRUCT_SHARE(ObjectDispatcher)
DECL_STRUCT_SHARE(MPlayerES)
DECL_STRUCT_SHARE(RocketryES)
DECL_STRUCT_SHARE(drawUnits)
namespace parserInt {
  class ParserShare {
  protected:
    static ecs::EntityManager &get_ecs(ParserState &state) { return state.g_entity_mgr; }
    static auto &get_unit_allocator(ParserState &state) { return state._unit_allocator; }
    static auto &get_battle_messages(ParserState &state) { return state.BattleMessages; }
    static auto &get_m_players(ParserState &state) { return state.players; }
    template<typename... Args>
    static void recordTeamSpawnSide(ParserState &state, Args &&...args) {
      state.recordTeamSpawnSide(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void setUnitData(ParserState &state, Args &&...args) {
      state.setUnitData(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static auto get_queued_data(ParserState &state, Args &&...args) {
      return state.get_queued_data(std::forward<Args>(args)...);
    }
    DECL_STRUCT_SHARE_FRIEND(BaseEntityES)
    DECL_STRUCT_SHARE_FRIEND(ObjectDispatcher)
    DECL_STRUCT_SHARE_FRIEND(MPlayerES)
    DECL_STRUCT_SHARE_FRIEND(RocketryES)
    DECL_STRUCT_SHARE_FRIEND(drawUnits)
  };
} // namespace parserInt
