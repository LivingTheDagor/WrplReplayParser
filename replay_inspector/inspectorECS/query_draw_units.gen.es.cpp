// Built with ECS codegen version 1.0
#include <ecs/query/entitySystem.h>
#include <ecs/componentTypes.h>
#include <ecs/ComponentTypesDefs.h>
#include "query_draw_units.cpp.inl"
ECS_DEF_PULL_VAR(query_draw_units);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc iterate_all_tanks_ecs_query_comps[] = {
  // start of 2 ro components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
  {ECS_HASH("unit__playerId"), ecs::ComponentTypeInfo<int>()},
  // start of 2 rq components at [2]
  {ECS_HASH("playerUnit"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("unit_tag__tank"), ecs::ComponentTypeInfo<ecs::Tag>()}};
static ecs::CompileTimeQueryDesc
  iterate_all_tanks_ecs_query_desc("iterate_all_tanks_ecs_query", ecs::empty_span(),
                                   ecs::make_span(iterate_all_tanks_ecs_query_comps + 0, 2) /*ro*/,
                                   ecs::make_span(iterate_all_tanks_ecs_query_comps + 2, 2) /*rq*/, ecs::empty_span());
template<typename Callable>
inline void iterate_all_tanks_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(&manager, iterate_all_tanks_ecs_query_desc.getHandle(),
                [&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
                  auto comp = components.begin(), compE = components.end();
                  G_ASSERT(comp != compE);
                  do
                    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
                      {
                        function(ECS_RO_COMP(iterate_all_tanks_ecs_query_comps, "unit__ref", unit::UnitRef),
                                 ECS_RO_COMP(iterate_all_tanks_ecs_query_comps, "unit__playerId", int), mgr);
                      }
                    }
                  while (++comp != compE);
                });
}
static constexpr ecs::ComponentDesc iterate_all_aircraft_ecs_query_comps[] = {
  // start of 2 ro components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
  {ECS_HASH("unit__playerId"), ecs::ComponentTypeInfo<int>()},
  // start of 2 rq components at [2]
  {ECS_HASH("playerUnit"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("unit_tag__aircraft"), ecs::ComponentTypeInfo<ecs::Tag>()}};
static ecs::CompileTimeQueryDesc
  iterate_all_aircraft_ecs_query_desc("iterate_all_aircraft_ecs_query", ecs::empty_span(),
                                      ecs::make_span(iterate_all_aircraft_ecs_query_comps + 0, 2) /*ro*/,
                                      ecs::make_span(iterate_all_aircraft_ecs_query_comps + 2, 2) /*rq*/,
                                      ecs::empty_span());
template<typename Callable>
inline void iterate_all_aircraft_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(&manager, iterate_all_aircraft_ecs_query_desc.getHandle(),
                [&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
                  auto comp = components.begin(), compE = components.end();
                  G_ASSERT(comp != compE);
                  do
                    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
                      {
                        function(ECS_RO_COMP(iterate_all_aircraft_ecs_query_comps, "unit__ref", unit::UnitRef),
                                 ECS_RO_COMP(iterate_all_aircraft_ecs_query_comps, "unit__playerId", int), mgr);
                      }
                    }
                  while (++comp != compE);
                });
}
static constexpr ecs::ComponentDesc iterate_all_player_units_ecs_query_comps[] = {
  // start of 1 ro components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
//start of 1 rq components at [1]
  {ECS_HASH("playerUnit"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc iterate_all_player_units_ecs_query_desc
(
  "iterate_all_player_units_ecs_query",
  ecs::empty_span(),
  ecs::make_span(iterate_all_player_units_ecs_query_comps+0, 1)/*ro*/,
  ecs::make_span(iterate_all_player_units_ecs_query_comps+1, 1)/*rq*/,
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_player_units_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(&manager, iterate_all_player_units_ecs_query_desc.getHandle(),
                [&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
                  auto comp = components.begin(), compE = components.end();
                  G_ASSERT(comp != compE);
                  do
                    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
                      {
                        function(ECS_RO_COMP(iterate_all_player_units_ecs_query_comps, "unit__ref", unit::UnitRef));
                      }
                    }
                  while (++comp != compE);
                });
}
static constexpr ecs::ComponentDesc iterate_all_rockets_ecs_query_comps[] = {
  // start of 1 ro components at [0]
  {ECS_HASH("rocket_component"), ecs::ComponentTypeInfo<Rocket>()}};
static ecs::CompileTimeQueryDesc
  iterate_all_rockets_ecs_query_desc("iterate_all_rockets_ecs_query", ecs::empty_span(),
                                     ecs::make_span(iterate_all_rockets_ecs_query_comps + 0, 1) /*ro*/,
                                     ecs::empty_span(), ecs::empty_span());
template<typename Callable>
inline void iterate_all_rockets_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(&manager, iterate_all_rockets_ecs_query_desc.getHandle(),
                [&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
                  auto comp = components.begin(), compE = components.end();
                  G_ASSERT(comp != compE);
                  do
                    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
                      {
                        function(ECS_RO_COMP(iterate_all_rockets_ecs_query_comps, "rocket_component", Rocket), mgr);
                      }
                    }
                  while (++comp != compE);
                });
}
static constexpr ecs::ComponentDesc iterate_all_bombs_ecs_query_comps[] = {
  // start of 1 ro components at [0]
  {ECS_HASH("bomb_component"), ecs::ComponentTypeInfo<Bomb>()}};
static ecs::CompileTimeQueryDesc iterate_all_bombs_ecs_query_desc("iterate_all_bombs_ecs_query", ecs::empty_span(),
                                                                  ecs::make_span(iterate_all_bombs_ecs_query_comps + 0,
                                                                                 1) /*ro*/,
                                                                  ecs::empty_span(), ecs::empty_span());
template<typename Callable>
inline void iterate_all_bombs_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(&manager, iterate_all_bombs_ecs_query_desc.getHandle(),
                [&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
                  auto comp = components.begin(), compE = components.end();
                  G_ASSERT(comp != compE);
                  do
                    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
                      {
                        function(ECS_RO_COMP(iterate_all_bombs_ecs_query_comps, "bomb_component", Bomb), mgr);

        }} while (++comp != compE);
    }
  );
}
