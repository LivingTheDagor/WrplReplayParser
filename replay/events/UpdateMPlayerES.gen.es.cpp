// Built with ECS codegen version 1.0
#include <ecs/query/entitySystem.h>
#include <ecs/componentTypes.h>
#include <ecs/ComponentTypesDefs.h>
#include "UpdateMPlayerES.cpp.inl"
ECS_DEF_PULL_VAR(UpdateMPlayer);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc mplayer_add_entity_es_comps[] = {
  // start of 3 ro components at [0]
  {ECS_HASH("unit__playerId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}};
static void mplayer_add_entity_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                             const ecs::QueryView &__restrict components) {
  if (evt.is<ecs::EventEntityDestroyedBasic>()) {
    auto comp = components.begin(), compE = components.end();
    G_ASSERT(comp != compE);
    do
      if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        mplayer_add_entity_es(static_cast<const ecs::EventEntityDestroyedBasic &>(evt),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "eid", ecs::EntityId),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__playerId", int),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__ref", unit::UnitRef), mgr);
      }
    while (++comp != compE);
  } else if (evt.is<ecs::EventEntityCreatedBasic>()) {
    auto comp = components.begin(), compE = components.end();
    G_ASSERT(comp != compE);
    do
      if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        mplayer_add_entity_es(static_cast<const ecs::EventEntityCreatedBasic &>(evt),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "eid", ecs::EntityId),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__playerId", int),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__ref", unit::UnitRef), mgr);
      }
    while (++comp != compE);
  } else if (evt.is<ecs::EventEntityCreated>()) {
    auto comp = components.begin(), compE = components.end();
    G_ASSERT(comp != compE);
    do
      if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        mplayer_add_entity_es(static_cast<const ecs::EventEntityCreated &>(evt),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__playerId", int),
                              ECS_RO_COMP(mplayer_add_entity_es_comps, "unit__ref", unit::UnitRef), mgr);
      }
    while (++comp != compE);
  } else {
    G_ASSERTF(0, "Unexpected event type <{}> in mplayer_add_entity_es", evt.getName());
  }
}
static ecs::EntitySystemDesc mplayer_add_entity_es_es_desc(
  "mplayer_add_entity_es", "D:/ReplayParser/replay/events/UpdateMPlayerES.cpp.inl",
  ecs::EntitySystemOps(mplayer_add_entity_es_all_events), ecs::empty_span(),
  ecs::make_span(mplayer_add_entity_es_comps + 0, 3) /*ro*/, ecs::empty_span(), ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated, ecs::EventEntityCreatedBasic, ecs::EventEntityDestroyedBasic>::build(),
  nullptr, nullptr, nullptr, "uid_entity_es");
