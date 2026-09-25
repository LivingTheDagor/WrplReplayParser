// Built with ECS codegen version 1.0
    #include <ecs/query/entitySystem.h>
    #include <ecs/componentTypes.h>
    #include <ecs/ComponentTypesDefs.h>
    #include "RocketryES.cpp.inl"
ECS_DEF_PULL_VAR(Rocketry);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc on_rocket_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("rocket_component"), ecs::ComponentTypeInfo<Rocket>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_rocket_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_rocket_appear_es(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(on_rocket_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(on_rocket_appear_es_comps, "rocket_component", Rocket)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_rocket_appear_es_es_desc
(
  "on_rocket_appear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_rocket_appear_es_all_events),
  ecs::make_span(on_rocket_appear_es_comps+0, 1)/*rw*/,
  ecs::make_span(on_rocket_appear_es_comps+1, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build()
);
static constexpr ecs::ComponentDesc on_rocket_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("rocket_component"), ecs::ComponentTypeInfo<Rocket>()}
};
static void on_rocket_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyedBasic>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_rocket_disappear_es(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
        , ECS_RW_COMP(on_rocket_disappear_es_comps, "rocket_component", Rocket)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_rocket_disappear_es_es_desc
(
  "on_rocket_disappear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_rocket_disappear_es_all_events),
  ecs::make_span(on_rocket_disappear_es_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyedBasic>::build()
);
static constexpr ecs::ComponentDesc on_bomb_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bomb_component"), ecs::ComponentTypeInfo<Bomb>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_bomb_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_bomb_appear_es(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(on_bomb_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(on_bomb_appear_es_comps, "bomb_component", Bomb)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_bomb_appear_es_es_desc
(
  "on_bomb_appear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_bomb_appear_es_all_events),
  ecs::make_span(on_bomb_appear_es_comps+0, 1)/*rw*/,
  ecs::make_span(on_bomb_appear_es_comps+1, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build()
);
static constexpr ecs::ComponentDesc on_bomb_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bomb_component"), ecs::ComponentTypeInfo<Bomb>()}
};
static void on_bomb_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyedBasic>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_bomb_disappear_es(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
        , ECS_RW_COMP(on_bomb_disappear_es_comps, "bomb_component", Bomb)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_bomb_disappear_es_es_desc
(
  "on_bomb_disappear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_bomb_disappear_es_all_events),
  ecs::make_span(on_bomb_disappear_es_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyedBasic>::build()
);
static constexpr ecs::ComponentDesc on_torpedo_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("torpedo_component"), ecs::ComponentTypeInfo<Torpedo>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_torpedo_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_torpedo_appear_es(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(on_torpedo_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(on_torpedo_appear_es_comps, "torpedo_component", Torpedo)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_torpedo_appear_es_es_desc
(
  "on_torpedo_appear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_torpedo_appear_es_all_events),
  ecs::make_span(on_torpedo_appear_es_comps+0, 1)/*rw*/,
  ecs::make_span(on_torpedo_appear_es_comps+1, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build()
);
static constexpr ecs::ComponentDesc on_torpedo_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("torpedo_component"), ecs::ComponentTypeInfo<Torpedo>()}
};
static void on_torpedo_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyedBasic>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_torpedo_disappear_es(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
        , ECS_RW_COMP(on_torpedo_disappear_es_comps, "torpedo_component", Torpedo)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_torpedo_disappear_es_es_desc
(
  "on_torpedo_disappear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_torpedo_disappear_es_all_events),
  ecs::make_span(on_torpedo_disappear_es_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyedBasic>::build()
);
static constexpr ecs::ComponentDesc on_payload_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("payload_component"), ecs::ComponentTypeInfo<Payload>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_payload_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_payload_appear_es(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(on_payload_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(on_payload_appear_es_comps, "payload_component", Payload)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_payload_appear_es_es_desc
(
  "on_payload_appear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_payload_appear_es_all_events),
  ecs::make_span(on_payload_appear_es_comps+0, 1)/*rw*/,
  ecs::make_span(on_payload_appear_es_comps+1, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build()
);
static constexpr ecs::ComponentDesc on_payload_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("payload_component"), ecs::ComponentTypeInfo<Payload>()}
};
static void on_payload_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyedBasic>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_payload_disappear_es(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
        , ECS_RW_COMP(on_payload_disappear_es_comps, "payload_component", Payload)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_payload_disappear_es_es_desc
(
  "on_payload_disappear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_payload_disappear_es_all_events),
  ecs::make_span(on_payload_disappear_es_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyedBasic>::build()
);
static constexpr ecs::ComponentDesc on_jettisoned_appear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jettisoned_component"), ecs::ComponentTypeInfo<Jettisoned>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void on_jettisoned_appear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_jettisoned_appear_es(static_cast<const ecs::EventEntityCreated&>(evt)
        , ECS_RO_COMP(on_jettisoned_appear_es_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(on_jettisoned_appear_es_comps, "jettisoned_component", Jettisoned)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_jettisoned_appear_es_es_desc
(
  "on_jettisoned_appear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_jettisoned_appear_es_all_events),
  ecs::make_span(on_jettisoned_appear_es_comps+0, 1)/*rw*/,
  ecs::make_span(on_jettisoned_appear_es_comps+1, 1)/*ro*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated>::build()
);
static constexpr ecs::ComponentDesc on_jettisoned_disappear_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jettisoned_component"), ecs::ComponentTypeInfo<Jettisoned>()}
};
static void on_jettisoned_disappear_es_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventEntityDestroyedBasic>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
    on_jettisoned_disappear_es(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
        , ECS_RW_COMP(on_jettisoned_disappear_es_comps, "jettisoned_component", Jettisoned)
    , mgr
    );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc on_jettisoned_disappear_es_es_desc
(
  "on_jettisoned_disappear_es",
  "D:/ReplayParser/replay/events/RocketryES.cpp.inl",
  ecs::EntitySystemOps(on_jettisoned_disappear_es_all_events),
  ecs::make_span(on_jettisoned_disappear_es_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyedBasic>::build()
);
static constexpr ecs::ComponentDesc iterate_all_rockets_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("rocket_component"), ecs::ComponentTypeInfo<Rocket>()}
};
static ecs::CompileTimeQueryDesc iterate_all_rockets_ecs_query_desc
(
  "iterate_all_rockets_ecs_query",
  ecs::make_span(iterate_all_rockets_ecs_query_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_rockets_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, iterate_all_rockets_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components, ecs::EntityManager &mgr)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        {
          function(
              ECS_RW_COMP(iterate_all_rockets_ecs_query_comps, "rocket_component", Rocket)
            );

        }} while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc iterate_all_torpedoes_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("torpedo_component"), ecs::ComponentTypeInfo<Torpedo>()}
};
static ecs::CompileTimeQueryDesc iterate_all_torpedoes_ecs_query_desc
(
  "iterate_all_torpedoes_ecs_query",
  ecs::make_span(iterate_all_torpedoes_ecs_query_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_torpedoes_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, iterate_all_torpedoes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components, ecs::EntityManager &mgr)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        {
          function(
              ECS_RW_COMP(iterate_all_torpedoes_ecs_query_comps, "torpedo_component", Torpedo)
            );

        }} while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc iterate_all_payloads_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("payload_component"), ecs::ComponentTypeInfo<Payload>()}
};
static ecs::CompileTimeQueryDesc iterate_all_payloads_ecs_query_desc
(
  "iterate_all_payloads_ecs_query",
  ecs::make_span(iterate_all_payloads_ecs_query_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_payloads_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, iterate_all_payloads_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components, ecs::EntityManager &mgr)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        {
          function(
              ECS_RW_COMP(iterate_all_payloads_ecs_query_comps, "payload_component", Payload)
            );

        }} while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc iterate_all_jettisoned_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jettisoned_component"), ecs::ComponentTypeInfo<Jettisoned>()}
};
static ecs::CompileTimeQueryDesc iterate_all_jettisoned_ecs_query_desc
(
  "iterate_all_jettisoned_ecs_query",
  ecs::make_span(iterate_all_jettisoned_ecs_query_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_jettisoned_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, iterate_all_jettisoned_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components, ecs::EntityManager &mgr)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        {
          function(
              ECS_RW_COMP(iterate_all_jettisoned_ecs_query_comps, "jettisoned_component", Jettisoned)
            );

        }} while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc iterate_all_bombs_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bomb_component"), ecs::ComponentTypeInfo<Bomb>()}
};
static ecs::CompileTimeQueryDesc iterate_all_bombs_ecs_query_desc
(
  "iterate_all_bombs_ecs_query",
  ecs::make_span(iterate_all_bombs_ecs_query_comps+0, 1)/*rw*/,
  ecs::empty_span(),
  ecs::empty_span(),
  ecs::empty_span());
template<typename Callable>
inline void iterate_all_bombs_ecs_query(ecs::EntityManager &manager, Callable function)
{
  perform_query(&manager, iterate_all_bombs_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components, ecs::EntityManager &mgr)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
        {
          function(
              ECS_RW_COMP(iterate_all_bombs_ecs_query_comps, "bomb_component", Bomb)
            );

        }} while (++comp != compE);
    }
  );
}
