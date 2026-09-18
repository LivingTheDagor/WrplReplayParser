#include "ecs/ecsCodegen.h"
#include "network/Connection.h"
#include "network/msgDecl.h"
#include "network/netEvent.h"
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


namespace ridestr {
  static void net_rcv_ri_destr_snapshot(const net::IMessage *msg);
  static void net_rcv_ri_destr_update(const net::IMessage *msg);
} // namespace ridestr

ECS_NET_DECL_MSG(RiDestrSnapshotMsg, BitStream);
ECS_NET_IMPL_MSG_HASH(RiDestrSnapshotMsg, net::ROUTING_SERVER_TO_CLIENT, 0xfa34bcf7, &net::broadcast_rcptf,
                      RELIABLE_ORDERED, 0, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &ridestr::net_rcv_ri_destr_snapshot);
ECS_NET_DECL_MSG(RiDestrUpdateMsg, BitStream);
ECS_NET_IMPL_MSG_HASH(RiDestrUpdateMsg, net::ROUTING_SERVER_TO_CLIENT, 0x2a247962, &net::direct_connection_rcptf,
                      RELIABLE_UNORDERED, 0, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &ridestr::net_rcv_ri_destr_update);


namespace ridestr {
  static void net_rcv_ri_destr_snapshot(const net::IMessage *msg) {
    auto ridestrMsg = msg->cast<RiDestrSnapshotMsg>();
    G_ASSERT(ridestrMsg);
    const BitStream &bs = ridestrMsg->get<0>();
    // bool done =
    //   rendinstdestr::deserialize_destr_data(bs, 0, rendinstdestr::get_destr_settings().riMaxSimultaneousDestrs);
    // G_ASSERT(!done || bs.GetNumberOfUnreadBits() == 0);
    // G_UNUSED(done);
  }

  static void net_rcv_ri_destr_update(const net::IMessage *msg) {
    // TIME_PROFILE(net_rcv_ri_destr_update);
    auto ridestrMsg = msg->cast<RiDestrUpdateMsg>();
    G_ASSERT(ridestrMsg);
    // bool done = rendinstdestr::deserialize_destr_update(ridestrMsg->get<0>());
    // G_ASSERT(!done || ridestrMsg->get<0>().GetNumberOfUnreadBits() == 0);
    // G_UNUSED(done);
  }
} // namespace ridestr
//

ECS_NET_DECL_MSG(GlobalPropsRegistryInitial, BitStream);
ECS_NET_DECL_MSG(GlobalPropsRegistryDiff, BitStream);
static void global_prop_reg_initial_handler(const net::IMessage *msg_) {
  return;
  auto msg = msg_->cast<GlobalPropsRegistryInitial>();
  G_ASSERT(msg);
  auto &bs = msg->get<0>();
  LOGI("[props] received message from server {} ({} bytes / {} items)", "GlobalPropsRegistryInitial",
       bs.GetWriteOffset(), BITS_TO_BYTES(bs.GetWriteOffset()) - 1);
}
static void global_prop_reg_diff_handler(const net::IMessage *msg_) {
  return;
  auto msg = msg_->cast<GlobalPropsRegistryDiff>();
  G_ASSERT(msg);
  auto &bs = msg->get<0>();
  LOGI("[props] received message from server {} ({} bytes / {} items)", "GlobalPropsRegistryDiff", bs.GetWriteOffset(),
       BITS_TO_BYTES(bs.GetWriteOffset()) - 1);
}

static void unk_event_handler(const net::IMessage *msg_) { LOGI("unk_event_handler called"); }


ECS_NET_IMPL_MSG(GlobalPropsRegistryInitial, net::ROUTING_SERVER_TO_CLIENT, &net::direct_connection_rcptf,
                 RELIABLE_ORDERED, 0, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &global_prop_reg_initial_handler);


ECS_NET_IMPL_MSG(GlobalPropsRegistryDiff, net::ROUTING_SERVER_TO_CLIENT, &net::direct_connection_rcptf,
                 RELIABLE_ORDERED, 0, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &global_prop_reg_diff_handler);

ECS_NET_DECL_MSG(UnkMessage, BitStream);

ECS_NET_IMPL_MSG_HASH(UnkMessage, net::ROUTING_SERVER_TO_CLIENT, 0x6c6b5b0a, &net::broadcast_rcptf, RELIABLE_ORDERED, 0,
                      net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &unk_event_handler);

static void ri_dester_info_handler(const net::IMessage *msg_) { LOGI("ri_dester_info called"); }

ECS_NET_DECL_MSG(RiDestrInfoMessage, BitStream);

ECS_NET_IMPL_MSG_HASH(RiDestrInfoMessage, net::ROUTING_SERVER_TO_CLIENT, 0x773e4201, &net::direct_connection_rcptf,
                      RELIABLE_ORDERED, 0, net::MF_DEFAULT_FLAGS, ECS_NET_NO_DUP, &ri_dester_info_handler);
// all NET EVENTS
struct uint8_t_3 {
  uint8_t data[3];
};
ECS_BROADCAST_EVENT_TYPE(EventShellVisualExplosion, uint8_t, net::compressed_uint32_t, uint16_t, bool, Point3,
                         uint8_t_3, uint8_t_3, bool, uint8_t, uint32_t, uint8_t, bool, bool, uint32_t);
ECS_REGISTER_EVENT(EventShellVisualExplosion)

ECS_REGISTER_NET_EVENT(EventShellVisualExplosion, net::Er::Broadcast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);

struct uint16_t_3 {
  uint16_t data[3];
};
ECS_UNICAST_EVENT_TYPE(EventFuelLeakEffectStart, uint16_t_3, Point3)
ECS_REGISTER_EVENT(EventFuelLeakEffectStart)
ECS_REGISTER_NET_EVENT(EventFuelLeakEffectStart, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);

ECS_UNICAST_EVENT_TYPE(EventDamagePartKilled, uint32_t, uint16_t)
ECS_REGISTER_EVENT(EventDamagePartKilled)
ECS_REGISTER_NET_EVENT(EventDamagePartKilled, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);

// not checked
ECS_UNICAST_EVENT_TYPE(EventTryControlBurav, ecs::EntityId, Point2) // I think Point2
ECS_REGISTER_EVENT(EventTryControlBurav)
ECS_REGISTER_NET_EVENT(EventTryControlBurav, net::Er::Unicast, net::ROUTING_CLIENT_TO_SERVER, &net::broadcast_rcptf);

// not checked
ECS_UNICAST_EVENT_TYPE(EventDiedNotReanimated)
ECS_REGISTER_EVENT(EventDiedNotReanimated)
ECS_REGISTER_NET_EVENT(EventDiedNotReanimated, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);

// this one probably exists, just isn't every use and synced maybe? its in client binary
// not checked
ECS_BROADCAST_EVENT_TYPE(EventServerDebugInfo, uint8_t, BitStream)
ECS_REGISTER_EVENT(EventServerDebugInfo)
ECS_REGISTER_NET_EVENT(EventServerDebugInfo, net::Er::Broadcast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf,
                       RELIABLE_ORDERED, 0, net::MF_OPTIONAL);

// not checked
ECS_UNICAST_EVENT_TYPE(EventFireSystemStartBurnWithOffenderInPos, uint16_t_3, uint32_t, uint32_t, uint16_t, uint32_t,
                       Point3)
ECS_REGISTER_EVENT(EventFireSystemStartBurnWithOffenderInPos)
ECS_REGISTER_NET_EVENT(EventFireSystemStartBurnWithOffenderInPos, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);

// not checked
ECS_UNICAST_EVENT_TYPE(EventOnGmFireTypeChangedChanged, uint8_t)
ECS_REGISTER_EVENT(EventOnGmFireTypeChangedChanged)
ECS_REGISTER_NET_EVENT(EventOnGmFireTypeChangedChanged, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);


static inline void event_gm_fire_type_changed_es_event_handler(const EventOnGmFireTypeChangedChanged &evt,
                                                               ecs::EntityId eid, ecs::EntityManager &manager) {
  return;
  LOGI("entity {}<{}> got EventOnGmFireTypeChangedChanged with value: {}", eid, manager.getEntityTemplateName(eid),
       evt.get<0>());
}
struct ShellData {
  uint8_t some_flags;
  uint8_t some_switch;
  char _padding[2]; // this and field below it actually contain ueseful data that I don't feel like parsing. the code is
                    // alot
  uint32_t _padd;
  uint32_t shell_id;
  Point2 some_point;
};
BOOST_DESCRIBE_STRUCT(ShellData, (), (some_flags, some_switch, _padding, _padd, shell_id, some_point))
G_STATIC_ASSERT(sizeof(ShellData) == sizeof(std::array<uint32_t, 5>));
// not checked
ECS_UNICAST_EVENT_TYPE(EventOnUnitLaunchShell, ShellData)
ECS_REGISTER_EVENT(EventOnUnitLaunchShell)
ECS_REGISTER_NET_EVENT(EventOnUnitLaunchShell, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);

static inline void event_unit_launch_shell_es_event_handler(const EventOnUnitLaunchShell &evt, ecs::EntityId eid,
                                                            ecs::EntityManager &manager, unit::UnitRef unit__ref) {
  return;
  LOGI("entity {}<{}> got EventOnUnitLaunchShell with value: {}", eid, manager.getEntityTemplateName(eid),
       toStringImplTyped(&evt.get<0>(), 0));
  if (unit__ref.unit) {
    auto weapon = unit__ref.unit->getWeaponFromRef(evt.get<0>().shell_id);
    LOGI("unit launching weapon of name {}", weapon ? weapon->weapon_name : "<unknown>");
  }
}


// not checked
ECS_UNICAST_EVENT_TYPE(EventRepairSystemNetSync, uint32_t, float, float, bool)
ECS_REGISTER_EVENT(EventRepairSystemNetSync)
ECS_REGISTER_NET_EVENT(EventRepairSystemNetSync, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);

static inline void event_repair_system_sync_es_event_handler(const EventRepairSystemNetSync &evt, ecs::EntityId eid,
                                                             ecs::EntityManager &manager) {
  return;
  LOGI("entity {}<{}> got EventRepairSystemNetSync with value: {}; {}; {}; {}", eid, manager.getEntityTemplateName(eid),
       evt.get<0>(), evt.get<1>(), evt.get<2>(), evt.get<3>());
}

ECS_BROADCAST_EVENT_TYPE(EventOnBombingZoneStateChanged, uint32_t)
ECS_REGISTER_EVENT(EventOnBombingZoneStateChanged)
ECS_REGISTER_NET_EVENT(EventOnBombingZoneStateChanged, net::Er::Broadcast, net::ROUTING_SERVER_TO_CLIENT,
                       &net::broadcast_rcptf);
static inline void event_bombing_zone_changed_es_event_handler(const EventOnBombingZoneStateChanged &evt) {
  LOGI("EventOnBombingZoneStateChanged arrived with value: {}", evt.get<0>());
}
ECS_UNICAST_EVENT_TYPE(EventDamagePartRestored, /*packed part id*/ uint32_t, float)
ECS_REGISTER_EVENT(EventDamagePartRestored)
ECS_REGISTER_NET_EVENT(EventDamagePartRestored, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);
static inline void event_damage_part_restored_es_event_handler(const EventDamagePartRestored &evt, ecs::EntityId eid,
                                                               ecs::EntityManager &manager) {
  return;
  LOGI("entity {}<{}> got EventDamagePartRestored with value: {}; {}", eid, manager.getEntityTemplateName(eid),
       evt.get<0>(), evt.get<1>());
}
ECS_UNICAST_EVENT_TYPE(EventBrakeTrack, Point3)
ECS_REGISTER_EVENT(EventBrakeTrack)
ECS_REGISTER_NET_EVENT(EventBrakeTrack, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);
static inline void event_brake_track_es_event_handler(const EventBrakeTrack &evt, ecs::EntityId eid,
                                                      ecs::EntityManager &manager) {
  LOGI("entity {}<{}> got EventBrakeTrack with value: {}", eid, manager.getEntityTemplateName(eid),
       toStringImplTyped(&evt.get<0>(), 0));
}
ECS_UNICAST_EVENT_TYPE(EventDoAmmoExplode, bool, bool, Point3, int32_t, uint32_t, Point2, uint32_t, uint32_t, uint32_t,
                       uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
ECS_REGISTER_EVENT(EventDoAmmoExplode)
ECS_REGISTER_NET_EVENT(EventDoAmmoExplode, net::Er::Unicast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);
static inline void event_do_ammo_explode_es_event_handler(const EventDoAmmoExplode &evt, ecs::EntityId eid,
                                                          ecs::EntityManager &manager) {
  LOGI("entity {}<{}> got EventDoAmmoExplode with value: {}", eid, manager.getEntityTemplateName(eid),
       toStringImplTyped(&evt.get<0>(), 0));
}
ECS_BROADCAST_EVENT_TYPE(EventSpawnBullet, bool, bool, bool, uint8_t, uint8_t, uint8_t, uint16_t, uint16_t,
                         net::compressed_uint32_t, net::compressed_uint32_t, net::compressed_uint32_t, Point3, Point3,
                         uint32_t, net::compressed_uint32_t, uint8_t)
ECS_REGISTER_EVENT(EventSpawnBullet)
ECS_REGISTER_NET_EVENT(EventSpawnBullet, net::Er::Broadcast, net::ROUTING_SERVER_TO_CLIENT, &net::broadcast_rcptf);
static inline void event_spawn_bullet_es_event_handler(const EventSpawnBullet &evt) {
  LOGI("EventSpawnBullet arrived with value: {}", toStringImplTyped(&evt.get<0>(), 0));
}
