// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <network/msgSink.h>
#include <network/msgDispatch.h>
#include <ecs/entityManager.h>
#include <ecs/query/entitySystem.h>
#include <ecs/query/coreEvents.h>
#include <network/message.h>
#include "ecs/ecsCodegen.h"
#include "state/ParserState.h"

namespace net {
  struct MsgSinkAccess {
    static ecs::EntityId get(const ParserState *s) { return s->msg_sink_eid; }
    static void set(ParserState *s, ecs::EntityId id) { s->msg_sink_eid = id; }
    static void reset(ParserState *s) { s->msg_sink_eid.reset(); }
  };
  msg_sink_created_cb_t on_msg_sink_created_cb;
  void set_msg_sink_created_cb(msg_sink_created_cb_t cb) { on_msg_sink_created_cb = eastl::move(cb); }

  ecs::EntityId get_msg_sink(ParserState *state) { return MsgSinkAccess::get(state); }

  ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "msg_sink", nullptr);

  ECS_REQUIRE(ecs::Tag msg_sink)
  static inline void msg_sink_es_event_handler(const ecs::EventEntityCreatedBasic &, ecs::EntityId eid,
                                               ecs::EntityManager &manager) {
    G_ASSERT(!MsgSinkAccess::get(manager.owned_by));
    MsgSinkAccess::set(manager.owned_by, eid);
    LOGI("Created with eid {}", eid);
    if (on_msg_sink_created_cb)
      on_msg_sink_created_cb(eid);
  }

  ECS_REQUIRE(ecs::Tag msg_sink)
  static inline void msg_sink_es_event_handler(const ecs::EventEntityDestroyedBasic &, ecs::EntityManager &manager) {
    G_ASSERT(MsgSinkAccess::get(manager.owned_by));
    MsgSinkAccess::reset(manager.owned_by);
  }

  ECS_REQUIRE(ecs::Tag msg_sink)
  static inline void msg_sink_es_event_handler(const ecs::EventNetMessage &evt) {
    dispatch_net_msg_handler(evt.get<0>().get());
  }

} // namespace net
