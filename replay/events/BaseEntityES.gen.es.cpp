// Built with ECS codegen version 1.0
#include <ecs/query/entitySystem.h>
#include <ecs/componentTypes.h>
#include <ecs/ComponentTypesDefs.h>
#include "BaseEntityES.cpp.inl"
ECS_DEF_PULL_VAR(BaseEntity);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc on_tank_appear_es_comps[] = {
  // start of 1 rw components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
//start of 2 ro components at [1]
  {ECS_HASH("unit_storage__tank"), ecs::ComponentTypeInfo<HeavyVehicleModelStorageComponent>()},
  {ECS_HASH("uid"), ecs::ComponentTypeInfo<int>()}};
static void on_tank_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                         const ecs::QueryView &__restrict components) {
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end();
  G_ASSERT(comp != compE);
  do
    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      on_tank_appear_es(static_cast<const ecs::EventEntityCreated &>(evt),
                        ECS_RO_COMP(on_tank_appear_es_comps, "unit_storage__tank", HeavyVehicleModelStorageComponent),
                        ECS_RO_COMP(on_tank_appear_es_comps, "uid", int),
                        ECS_RW_COMP(on_tank_appear_es_comps, "unit__ref", unit::UnitRef), mgr);
    }
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_tank_appear_es_es_desc("on_tank_appear_es",
                                                       "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                                                       ecs::EntitySystemOps(on_tank_appear_es_all_events),
                                                       ecs::make_span(on_tank_appear_es_comps + 0, 1) /*rw*/,
                                                       ecs::make_span(on_tank_appear_es_comps + 1, 2) /*ro*/,
                                                       ecs::empty_span(), ecs::empty_span(),
                                                       ecs::EventSetBuilder<ecs::EventEntityCreated>::build());
static constexpr ecs::ComponentDesc on_aircraft_appear_es_comps[] = {
  // start of 1 rw components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
//start of 2 ro components at [1]
  {ECS_HASH("unit_storage__aircraft"), ecs::ComponentTypeInfo<FlightModelWrapStorageComponent>()},
  {ECS_HASH("uid"), ecs::ComponentTypeInfo<int>()}};
static void on_aircraft_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                             const ecs::QueryView &__restrict components) {
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end();
  G_ASSERT(comp != compE);
  do
    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      on_aircraft_appear_es(
        static_cast<const ecs::EventEntityCreated &>(evt),
        ECS_RO_COMP(on_aircraft_appear_es_comps, "unit_storage__aircraft", FlightModelWrapStorageComponent),
        ECS_RO_COMP(on_aircraft_appear_es_comps, "uid", int),
        ECS_RW_COMP(on_aircraft_appear_es_comps, "unit__ref", unit::UnitRef), mgr);
    }
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_aircraft_appear_es_es_desc("on_aircraft_appear_es",
                                                           "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                                                           ecs::EntitySystemOps(on_aircraft_appear_es_all_events),
                                                           ecs::make_span(on_aircraft_appear_es_comps + 0, 1) /*rw*/,
                                                           ecs::make_span(on_aircraft_appear_es_comps + 1, 2) /*ro*/,
                                                           ecs::empty_span(), ecs::empty_span(),
                                                           ecs::EventSetBuilder<ecs::EventEntityCreated>::build());
static constexpr ecs::ComponentDesc on_unit_disappear_es_comps[] = {
  // start of 1 rw components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}};
static void on_unit_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                            const ecs::QueryView &__restrict components) {
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyed>());
  auto comp = components.begin(), compE = components.end();
  G_ASSERT(comp != compE);
  do
    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      on_unit_disappear_es(static_cast<const ecs::EventEntityDestroyed &>(evt),
                           ECS_RW_COMP(on_unit_disappear_es_comps, "unit__ref", unit::UnitRef));
    }
  while (++comp != compE);
}
static ecs::EntitySystemDesc on_unit_disappear_es_es_desc("on_unit_disappear_es",
                                                          "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                                                          ecs::EntitySystemOps(on_unit_disappear_es_all_events),
                                                          ecs::make_span(on_unit_disappear_es_comps + 0, 1) /*rw*/,
                                                          ecs::empty_span(), ecs::empty_span(), ecs::empty_span(),
                                                          ecs::EventSetBuilder<ecs::EventEntityDestroyed>::build());
static constexpr ecs::ComponentDesc after_unit_appear_es_comps[] = {
  // start of 1 rw components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
//start of 1 ro components at [1]
  {ECS_HASH("uid"), ecs::ComponentTypeInfo<int>()}
};
static void after_unit_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end();
  G_ASSERT(comp != compE);
  do
    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      after_unit_appear_es(static_cast<const ecs::EventEntityCreated &>(evt),
                           ECS_RW_COMP(after_unit_appear_es_comps, "unit__ref", unit::UnitRef),
                           ECS_RO_COMP(after_unit_appear_es_comps, "uid", int), mgr);
    }
  while (++comp != compE);
}
static ecs::EntitySystemDesc
  after_unit_appear_es_es_desc("after_unit_appear_es", "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                               ecs::EntitySystemOps(after_unit_appear_es_all_events),
                               ecs::make_span(after_unit_appear_es_comps + 0, 1) /*rw*/,
                               ecs::make_span(after_unit_appear_es_comps + 1, 1) /*ro*/, ecs::empty_span(),
                               ecs::empty_span(), ecs::EventSetBuilder<ecs::EventEntityCreated>::build(), nullptr,
                               nullptr, nullptr, "on_aircraft_appear_es,on_tank_appear_es");
static constexpr ecs::ComponentDesc uid_entity_es_comps[] = {
  // start of 1 rw components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
//start of 2 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("uid"), ecs::ComponentTypeInfo<int>()}};
static void uid_entity_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                     const ecs::QueryView &__restrict components) {
  if (evt.is<ecs::EventEntityDestroyedBasic>()) {
    auto comp = components.begin(), compE = components.end();
    G_ASSERT(comp != compE);
    do
      if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        uid_entity_es(static_cast<const ecs::EventEntityDestroyedBasic &>(evt),
                      ECS_RW_COMP(uid_entity_es_comps, "unit__ref", unit::UnitRef),
                      ECS_RO_COMP(uid_entity_es_comps, "uid", int), mgr);
      }
    while (++comp != compE);
  } else if (evt.is<ecs::EventEntityCreatedBasic>()) {
    auto comp = components.begin(), compE = components.end();
    G_ASSERT(comp != compE);
    do
      if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        uid_entity_es(static_cast<const ecs::EventEntityCreatedBasic &>(evt),
                      ECS_RO_COMP(uid_entity_es_comps, "eid", ecs::EntityId),
                      ECS_RO_COMP(uid_entity_es_comps, "uid", int),
                      ECS_RW_COMP(uid_entity_es_comps, "unit__ref", unit::UnitRef), mgr);
      }
    while (++comp != compE);
  } else {
    G_ASSERTF(0, "Unexpected event type <{}> in uid_entity_es", evt.getName());
  }
}
static ecs::EntitySystemDesc
  uid_entity_es_es_desc("uid_entity_es", "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                        ecs::EntitySystemOps(uid_entity_es_all_events),
                        ecs::make_span(uid_entity_es_comps + 0, 1) /*rw*/,
                        ecs::make_span(uid_entity_es_comps + 1, 2) /*ro*/, ecs::empty_span(), ecs::empty_span(),
                        ecs::EventSetBuilder<ecs::EventEntityCreatedBasic, ecs::EventEntityDestroyedBasic>::build(),
                        nullptr, nullptr, nullptr, "after_unit_appear_es");
static constexpr ecs::ComponentDesc on_unit_appear_mpi_es_comps[] = {
  // start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}};
static void on_unit_appear_mpi_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt,
                                             const ecs::QueryView &__restrict components) {
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end();
  G_ASSERT(comp != compE);
  do
    if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      on_unit_appear_mpi_es(static_cast<const ecs::EventEntityCreated &>(evt),
                            ECS_RO_COMP(on_unit_appear_mpi_es_comps, "eid", ecs::EntityId),
                            ECS_RO_COMP(on_unit_appear_mpi_es_comps, "unit__ref", unit::UnitRef), mgr);
    }
  while (++comp != compE);
}
static ecs::EntitySystemDesc
  on_unit_appear_mpi_es_es_desc("on_unit_appear_mpi_es", "D:/ReplayParser/replay/events/BaseEntityES.cpp.inl",
                                ecs::EntitySystemOps(on_unit_appear_mpi_es_all_events), ecs::empty_span(),
                                ecs::make_span(on_unit_appear_mpi_es_comps + 0, 2) /*ro*/, ecs::empty_span(),
                                ecs::empty_span(), ecs::EventSetBuilder<ecs::EventEntityCreated>::build(), nullptr,
                                nullptr, nullptr, "after_unit_appear_es");
static constexpr ecs::ComponentDesc iterate_all_units_ecs_query_comps[] = {
  // start of 3 ro components at [0]
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()},
  {ECS_HASH("unit__className"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [3]
  {ECS_HASH("playerUnit"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc iterate_all_units_ecs_query_desc
(
  "iterate_all_units_ecs_query",
  ecs::empty_span(),
  ecs::make_span(iterate_all_units_ecs_query_comps+0, 3)/*ro*/,
  ecs::make_span(iterate_all_units_ecs_query_comps+3, 1)/*rq*/,
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_units_ecs_query(ecs::EntityManager &manager, Callable function) {
  perform_query(
    &manager, iterate_all_units_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView &__restrict components, ecs::EntityManager &mgr) {
      auto comp = components.begin(), compE = components.end();
      G_ASSERT(comp != compE);
      do
        if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
          {
            if (function(ECS_RO_COMP(iterate_all_units_ecs_query_comps, "unit__ref", unit::UnitRef),
                         ECS_RO_COMP(iterate_all_units_ecs_query_comps, "unit__className", ecs::string),
                         ECS_RO_COMP(iterate_all_units_ecs_query_comps, "eid", ecs::EntityId)) ==
                ecs::QueryCbResult::Stop)
              return ecs::QueryCbResult::Stop;
          }
        }
      while (++comp != compE);
      return ecs::QueryCbResult::Continue;
    }));
}
