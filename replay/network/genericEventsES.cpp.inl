#include "network/Connection.h"
#include "network/msgDecl.h"
#include "network/serialize.h"
#include "network/schemelessEventSerialize.h"

static constexpr int NET_SQ_EVENTS_DEFAULT_NET_CHANNEL =
  0; // Note: this might be unresolved link time dependency if apps really need
// to configure that

ECS_NET_DECL_MSG(SQEventMsg, bool /*bcast*/, ecs::EntityId /*to_eid*/, BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientSQEventMsg, bool /*bcast*/, ecs::EntityId /*to_eid*/, BitStream /*payload*/);

template<typename T>
static void sq_event_msg_handler(const net::IMessage *msg_) {
  net::Connection *conn = msg_->connection;
  auto mgr = conn->getEntityManager();
  auto msg = msg_->cast<T>();
  G_ASSERT(msg);
  if (ecs::MaybeSchemelessEvent maybeEvent = ecs::deserialize_from(*mgr, msg->template get<2>())) {
    // To consider: add non-payload field to native SchemelessEvent instead?
    maybeEvent->getRWData().addMember(ECS_HASH("fromconnid"), -1);
    if (msg->template get<0>()) // bcast
      mgr->broadcastEventImmediate(eastl::move(eastl::move(maybeEvent).value()));
    else {
      if (ecs::EntityId to_eid = msg->template get<1>())
        mgr->sendEventImmediate(to_eid, eastl::move(eastl::move(maybeEvent).value()));
      else
        LOGE("failed to resolve destination entity for SQEvent '{}'", maybeEvent.value().getName());
    }
  } else
    LOGE("Failed to parse SQEventMsg");
}

ECS_NET_IMPL_MSG(SQEventMsg, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED,
                 NET_SQ_EVENTS_DEFAULT_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &sq_event_msg_handler<SQEventMsg>);
ECS_NET_IMPL_MSG(ClientSQEventMsg, net::ROUTING_CLIENT_TO_SERVER, ECS_NET_NO_RCPTF, RELIABLE_ORDERED,
                 NET_SQ_EVENTS_DEFAULT_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &sq_event_msg_handler<ClientSQEventMsg>);

// das events
static constexpr int DAS_NET_EVENTS_DEF_NET_CHANNEL =
  0; // Note: this might be unresolved link time dependency if apps really need to
// configure that
static constexpr int DAS_NET_EVENTS_MAX_PLAYERS = 128;

ECS_NET_DECL_MSG(DasEventMsg, BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientDasEventMsg, BitStream /*payload*/);
ECS_NET_DECL_MSG(ClientControlledEntityDasEventMsg, BitStream /*payload*/);

ECS_NET_DECL_MSG(DasEventHandshakeMsg, /*proto_version*/ uint32_t);

namespace net {
  enum {
    DAS_EVENT_HASH_BITS = 22,
    DAS_EVENT_HASH_MASK = (1 << DAS_EVENT_HASH_BITS) - 1,
    EVENT_STACK_SIZE = 256,
  };
  template<class EventName, typename CB>
  static void base_das_event_msg_handler(const EventName *msg, CB send_event) {
    G_ASSERT(msg);
    // invalidate_das_events_gen("event gen");
    ecs::EntityManager &mgr = *msg->connection->getEntityManager();
    alignas(16) char buf[EVENT_STACK_SIZE];
    const BitStream bs = msg->template get<0>();
    /*
    if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE))
    {
      TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
      send_event((ecs::Event &)*evt);
      if (DAGOR_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is
    immediate strategy. mgr.getEventsDb().destroy(mgr, *evt); if ((char *)evt != buf) framemem_ptr()->free(evt);
    }*/
  }

  template<class EventName>
  static void broadcast_das_event_msg_handler(const net::IMessage *msg_) {
    base_das_event_msg_handler(msg_->cast<EventName>(), [msg_](ecs::Event &evt) {
      msg_->connection->getEntityManager()->broadcastEventImmediate(evt);
    });
  }
  template<class EventName, typename CB>
  static void base_client_das_event_msg_handler(const EventName *msg, CB send_event) {
    G_ASSERT(msg);
    // invalidate_das_events_gen("client event gen");
    ecs::EntityManager &mgr = *msg->connection->getEntityManager();
    alignas(16) char buf[EVENT_STACK_SIZE];
    const BitStream bs = msg->template get<0>();
    /*
    if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE))
    {
      TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
      send_event((ecs::Event &)*evt);
      if (DAGOR_UNLIKELY(evt->getFlags() & ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is
    immediate strategy. mgr.getEventsDb().destroy(mgr, *evt); if ((char *)evt != buf) framemem_ptr()->free(evt);
    }*/
  }

  template<class EventName>
  static void broadcast_client_das_event_msg_handler(const net::IMessage *msg_) {
    base_client_das_event_msg_handler(msg_->cast<EventName>(), [msg_](ecs::Event &evt) {
      msg_->connection->getEntityManager()->broadcastEventImmediate(evt);
    });
  }
  template<class EventName, typename CB>
  static void base_client_controlled_das_event_msg_handler(const EventName *msg, CB send_event) {
    G_ASSERT(msg);
    // invalidate_das_events_gen("ctrl event gen");
    ecs::EntityManager &mgr = *msg->connection->getEntityManager();
    alignas(16) char buf[EVENT_STACK_SIZE];
    const BitStream bs = msg->template get<0>();
    // this is actual das logic WHICH I DONT HAVE AND DON'T PLAN TO SETUP
    /*if (bind_dascript::DasEvent *evt = deserialize_das_event(bs, msg, buf, EVENT_STACK_SIZE)) {
      // TRACE_RX_DAS_EVENT_STAT(evt->getType(), bs);
      send_event((ecs::Event &) *evt);
      if (DAGOR_UNLIKELY(evt->getFlags() &
                         ecs::EVFLG_DESTROY)) // we have to do it, as it can be that there is immediate strategy.
        ecs::g_ecs_data->getEventsDb().destroy(*evt);
      if ((char *) evt != buf);
        //framemem_ptr()->free(evt);
    }*/
  }

  template<class EventName>
  static void broadcast_client_controlled_das_event_msg_handler(const net::IMessage *msg_) {
    base_client_controlled_das_event_msg_handler(msg_->cast<EventName>(), [msg_](ecs::Event &evt) {
      msg_->connection->getEntityManager()->broadcastEventImmediate(evt);
    });
  }

  template<class EventName>
  static void unicast_client_controlled_das_event_msg_handler(const ecs::EntityId &eid, const EventName *msg_) {
    base_client_controlled_das_event_msg_handler(
      msg_, [&](ecs::Event &evt) { msg_->connection->getEntityManager()->sendEventImmediate(eid, evt); });
  }
  static void das_event_handshake_msg_handler(const net::IMessage *msg_) {
    auto msg = msg_->cast<DasEventHandshakeMsg>();
    G_ASSERT(msg);
    const uint32_t clientVersion = msg->get<0>();
    /*const uint32_t serverVersion = bind_dascript::lock_dasevent_net_version(msg_->connection->getEntityManager());
    if (clientVersion == serverVersion)
    {
      debug("das_net: client #%d has correct dasevents version protocol 0x%x", (int)msg_->connection->getId(),
    serverVersion);
    }
    else
    {
      logwarn("das_net: Invalid client #%d dasevents version 0x%x (vs 0x%x)", (int)msg_->connection->getId(),
    clientVersion, serverVersion); msg_->connection->disconnect(DC_NET_PROTO_MISMATCH);
    }*/
    LOGI("das_net: dasevents version protocol 0x{:x}", clientVersion);
  }

  template<class EventMsg>
  static eastl::string format_das_event_msg_str(const net::IMessage *msg) {
    const net::MessageClass &msgCls = msg->getMsgClass();
    auto e = msg->cast<EventMsg>();
    G_ASSERT_RETURN(e, net::MessageClass::defaultFormatMsgStr(msg));
    ecs::event_type_t eventType = 0;
    e->template get<0>().Read(eventType);
    const auto &eventsDb = ecs::g_ecs_data->getEventsDb();
    const auto eventId = eventsDb.findEvent(eventType);
    eastl::string result;
    if (eventId != ecs::EventsDB::invalid_event_id)
      result.sprintf("#%d/%s/%x (%s(0x%X))", msgCls.classId, msgCls.debugClassName, msgCls.classHash,
                     eventsDb.getEventName(eventId), eventType);
    else
      result.sprintf("#%d/%s/%x (0x%X)", msgCls.classId, msgCls.debugClassName, msgCls.classHash, eventType);
    return result;
  }

} // namespace net


ECS_NET_IMPL_MSG(DasEventMsg, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf, RELIABLE_ORDERED,
                 DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &net::broadcast_das_event_msg_handler<DasEventMsg>);

ECS_NET_IMPL_MSG(ClientDasEventMsg, net::ROUTING_CLIENT_TO_SERVER, ECS_NET_NO_RCPTF, RELIABLE_ORDERED,
                 DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &net::broadcast_client_das_event_msg_handler<ClientDasEventMsg>,
                 &net::format_das_event_msg_str<ClientDasEventMsg>);

ECS_NET_IMPL_MSG(ClientControlledEntityDasEventMsg, net::ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER, ECS_NET_NO_RCPTF,
                 RELIABLE_ORDERED, DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &net::broadcast_client_controlled_das_event_msg_handler<ClientControlledEntityDasEventMsg>,
                 &net::format_das_event_msg_str<ClientControlledEntityDasEventMsg>);

ECS_NET_IMPL_MSG(DasEventHandshakeMsg, net::ROUTING_CLIENT_TO_SERVER, &net::broadcast_rcptf, RELIABLE_ORDERED,
                 DAS_NET_EVENTS_DEF_NET_CHANNEL, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP,
                 &net::das_event_handshake_msg_handler);


#ECS_NET_DECL_MSG(SomeWeirdMsg, uint8_t, );
