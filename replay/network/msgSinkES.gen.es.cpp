// Built with ECS codegen version 1.0
    #include <ecs/query/entitySystem.h>
    #include <ecs/componentTypes.h>
    #include <ecs/ComponentTypesDefs.h>
    #include "msgSinkES.cpp.inl"
ECS_DEF_PULL_VAR(msgSink);
#include <ecs/query/performQuery.h>
static constexpr ecs::ComponentDesc msg_sink_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void msg_sink_es_event_handler_all_events(ecs::EntityManager &mgr, const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<ecs::EventNetMessage>()) {
    net::msg_sink_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
            );
} else if (evt.is<ecs::EventEntityDestroyedBasic>()) {
    net::msg_sink_es_event_handler(static_cast<const ecs::EventEntityDestroyedBasic&>(evt)
            , mgr
      );
} else if (evt.is<ecs::EventEntityCreatedBasic>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do if (components.eid_refs[comp] != ecs::INVALID_ENTITY_ID) {
      net::msg_sink_es_event_handler(static_cast<const ecs::EventEntityCreatedBasic&>(evt)
            , ECS_RO_COMP(msg_sink_es_event_handler_comps, "eid", ecs::EntityId)
      , mgr
      );
    } while (++comp != compE);
    } else {G_ASSERTF(0, "Unexpected event type <{}> in msg_sink_es_event_handler", evt.getName());}
}
static ecs::EntitySystemDesc msg_sink_es_event_handler_es_desc
(
  "msg_sink_es",
  "D:/ReplayParser/replay/network/msgSinkES.cpp.inl",
  ecs::EntitySystemOps(msg_sink_es_event_handler_all_events),
  ecs::empty_span(),
  ecs::make_span(msg_sink_es_event_handler_comps+0, 1)/*ro*/,
  ecs::make_span(msg_sink_es_event_handler_comps+1, 1)/*rq*/,
  ecs::empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreatedBasic,
                       ecs::EventEntityDestroyedBasic,
                       ecs::EventNetMessage>::build()
);
