// Built with ECS codegen version 1.0
    #include <ecs/query/entitySystem.h>
    #include <ecs/componentTypes.h>
    #include <ecs/ComponentTypesDefs.h>
    #include "genericEventsES.cpp.inl"
ECS_DEF_PULL_VAR(genericEvents);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc event_gm_fire_type_changed_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void event_gm_fire_type_changed_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnGmFireTypeChangedChanged>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_gm_fire_type_changed_es_event_handler(static_cast<const EventOnGmFireTypeChangedChanged&>(evt)
        , ECS_RO_COMP(event_gm_fire_type_changed_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_gm_fire_type_changed_es_event_handler_es_desc
(
  "event_gm_fire_type_changed_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_gm_fire_type_changed_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_gm_fire_type_changed_es_event_handler_comps+0, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventOnGmFireTypeChangedChanged>::build()
);
static constexpr ecs::ComponentDesc event_unit_launch_shell_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}
};
static void event_unit_launch_shell_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventOnUnitLaunchShell>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_unit_launch_shell_es_event_handler(static_cast<const EventOnUnitLaunchShell&>(evt)
        , ECS_RO_COMP(event_unit_launch_shell_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    , ECS_RO_COMP(event_unit_launch_shell_es_event_handler_comps, "unit__ref", unit::UnitRef)
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_unit_launch_shell_es_event_handler_es_desc
(
  "event_unit_launch_shell_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_unit_launch_shell_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_unit_launch_shell_es_event_handler_comps+0, 2)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventOnUnitLaunchShell>::build()
);
static constexpr ecs::ComponentDesc event_repair_system_sync_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void event_repair_system_sync_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventRepairSystemNetSync>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_repair_system_sync_es_event_handler(static_cast<const EventRepairSystemNetSync&>(evt)
        , ECS_RO_COMP(event_repair_system_sync_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_repair_system_sync_es_event_handler_es_desc
(
  "event_repair_system_sync_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_repair_system_sync_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_repair_system_sync_es_event_handler_comps+0, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventRepairSystemNetSync>::build()
);
//static constexpr ecs::ComponentDesc event_bombing_zone_changed_es_event_handler_comps[] ={};
static void event_bombing_zone_changed_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnBombingZoneStateChanged>());
  event_bombing_zone_changed_es_event_handler(static_cast<const EventOnBombingZoneStateChanged&>(evt)
        );
}
static ecs::EntitySystemDesc event_bombing_zone_changed_es_event_handler_es_desc
(
  "event_bombing_zone_changed_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_bombing_zone_changed_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventOnBombingZoneStateChanged>::build()
);
static constexpr ecs::ComponentDesc event_damage_part_restored_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void event_damage_part_restored_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventDamagePartRestored>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_damage_part_restored_es_event_handler(static_cast<const EventDamagePartRestored&>(evt)
        , ECS_RO_COMP(event_damage_part_restored_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_damage_part_restored_es_event_handler_es_desc
(
  "event_damage_part_restored_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_damage_part_restored_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_damage_part_restored_es_event_handler_comps+0, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventDamagePartRestored>::build()
);
static constexpr ecs::ComponentDesc event_brake_track_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void event_brake_track_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventBrakeTrack>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_brake_track_es_event_handler(static_cast<const EventBrakeTrack&>(evt)
        , ECS_RO_COMP(event_brake_track_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_brake_track_es_event_handler_es_desc
(
  "event_brake_track_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_brake_track_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_brake_track_es_event_handler_comps+0, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventBrakeTrack>::build()
);
static constexpr ecs::ComponentDesc event_do_ammo_explode_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void event_do_ammo_explode_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventDoAmmoExplode>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    event_do_ammo_explode_es_event_handler(static_cast<const EventDoAmmoExplode&>(evt)
        , ECS_RO_COMP(event_do_ammo_explode_es_event_handler_comps, "eid", ecs::EntityId)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc event_do_ammo_explode_es_event_handler_es_desc
(
  "event_do_ammo_explode_es",
  "D:/ReplayParser/replay/network/genericEventsES.cpp.inl",
  ecs::EntitySystemOps(event_do_ammo_explode_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(event_do_ammo_explode_es_event_handler_comps+0, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<EventDoAmmoExplode>::build()
);
