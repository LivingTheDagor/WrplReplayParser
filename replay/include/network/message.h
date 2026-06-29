//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>
#include <EASTL/memory.h>
#include <daNet/packetPriority.h>
#include <daNet/daNetTypes.h> // DaNetTime
#include <utils.h>
#include <ecs/query/event.h>
// #include <util/dag_hash.h>
#include "dag_assert.h"

class BitStream;


namespace net {

  class Object;
  class MessageClass;
  class IConnection;

  enum MessageRouting : uint8_t {
    ROUTING_CLIENT_TO_SERVER,
    ROUTING_SERVER_TO_CLIENT,
    ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER // Only entity whose 'replication' component is marked via
                                               // 'setControlledBy()' will get
    // this kind of messages
  };

  enum MessageFlags : uint8_t {
    MF_URGENT = 1 << 0, // Wake-up network thread for sending this message
    MF_DISCARD_IF_NO_ENTITY =
      1 << 1, // Don't queue this message up if there is no entity to deliver it to (on reception)
    MF_DONT_COMPRESS = 1 << 2, // Don't try to compress this message
    MF_TIMED = 1 << 3, // Actual message class is descendant of IMessageTimed (and will get 'rcvTime')
    MF_OPTIONAL = 1 << 4, // Does not influence net proto (unless there are no free slots in `numClassIdBits`)

    MF_DEFAULT_FLAGS = 0,
  };

  class IMessage {
  public:
    // On receive - which connection this message is received from (can't be null in network)
    // On send (for some rcptf) - which connection this message need to be sent to
    IConnection
      *connection; // Note: intentionally not inited by default (to save CPU, since in most cases it's not used/always
    // overwritten)

    IMessage() = default;
    IMessage(const IMessage &) = default;
    virtual ~IMessage() = default;

    virtual const MessageClass &getMsgClass() const = 0;

    virtual void pack(BitStream &bs) const = 0;
    virtual bool unpack(const BitStream &bs, const net::IConnection &conn) = 0;

    virtual IMessage *moveHeap() && = 0;

    template<typename T>
    const T *cast() const {
      return &getMsgClass() == &T::messageClass ? static_cast<const T *>(this) : nullptr;
    }
  };

  class IMessageTimed : public IMessage {
  public:
    DaNetTime rcvTime = 0; // time when message received
    using IMessage::IMessage;
  };

  class MessageDeleter {
    bool heapAllocated;

  public:
    MessageDeleter(bool heap) : heapAllocated(heap) {}
    void operator()(IMessage *ptr) {
      if (!ptr)
        return;
      if (heapAllocated)
        delete ptr;
      else
        eastl::destroy_at(ptr);
    }
  };

  typedef std::span<net::IConnection *> (*recipient_filter_t)(std::vector<net::IConnection *> &out_conns,
                                                              ecs::EntityId to_eid, const net::IMessage &);

// 'rcptf' is abbreviation of 'recipient filter'
#ifdef _MSC_VER
  __declspec(noinline)
#endif
  // aka no filter (do not inline it to force compiler generate distinct address for this function)
  std::span<net::IConnection *> broadcast_rcptf(std::vector<net::IConnection *> &out_conns, ecs::EntityId,
                                                const IMessage &)
#ifdef __GNUC__ /* including clang */
    __attribute__((noinline))
#endif
    ;
  std::span<net::IConnection *> direct_connection_rcptf(std::vector<net::IConnection *> &out_conns, ecs::EntityId,
                                                        const IMessage &);

#define ECS_NET_NO_RCPTF nullptr
#define ECS_NET_NO_DUP   0

  struct MessageNetDesc {
    MessageRouting routing;
    PacketReliability reliability;
    uint8_t channel;
    eastl::underlying_type_t<MessageFlags> flags;
    uint16_t dupDelay; // If not 0 - message will be duplicated (resent) after this number of milliseconds
    recipient_filter_t rcptFilter;
  };

  class MessageClass : public MessageNetDesc {
    MessageClass *next;
    static MessageClass *classLinkList;
    static int numClassIdBits;

  public:
    void (*msgSinkHandler)(
      const IMessage *msg); // If not null then this handler will be called on msgSink messages receival
    const char *debugClassName = nullptr; // null in release (see ECS_NET_MSG_CLASS_NAME)
    uint32_t classHash;
    int16_t classId = -1;
    uint16_t memSize;

    MessageClass(const char *class_name, uint32_t class_hash, uint32_t class_sz, MessageRouting rout, bool timed,
                 recipient_filter_t rcptf = ECS_NET_NO_RCPTF, PacketReliability rlb = RELIABLE_ORDERED, uint8_t chn = 0,
                 uint32_t flags_ = MF_DEFAULT_FLAGS, int dup_delay_ms = ECS_NET_NO_DUP,
                 void (*msg_sink_handler)(const IMessage *) = nullptr);

  private:
    static uint32_t initImpl(bool server, bool dbg_output_table);

  public:
    static uint32_t init(bool server);

    static void startWaitingForMessageIdsSync();
    static void resetMessageIdsSync();
    static void writeMessageIdsSync(BitStream &bs, uint64_t client_hash = 0u);
    static bool applyMessageIdsSync(const BitStream &bs);

    static const MessageClass *getById(int msg_class_id);
    static int getNumClassIdBits();
    static uint32_t calcNumMessageClasses();
    static uint64_t calcAllMessagesHash();
    static bool validateIncomingMessage(int msg_class_id, int dbg_conn_id);
    static bool shouldIgnoreOutgoingMessage(int msg_class_id);

    virtual IMessage &create(void *mem) const = 0;
  };

  template<typename T>
  class MessageClassInst final : public MessageClass {
  public:
    G_STATIC_ASSERT(sizeof(T) <= eastl::numeric_limits<decltype(MessageClass::memSize)>::max());
    using MessageClass::MessageClass;
    MessageClassInst() = delete;
    IMessage &create(void *mem) const override { return *new (mem) T{}; }
  };


#if DAGOR_DBGLEVEL > 0
#define ECS_NET_MSG_CLASS_NAME(x) #x
#else
#define ECS_NET_MSG_CLASS_NAME(x) \
  nullptr // No message names in release to complicate reverse-engineering (and reduce size of final executable)
#endif
#define ECS_NET_MSG_CLASS_HASH(a) eastl::integral_constant<uint32_t, str_hash_fnv1(#a)>::value

#define ECS_NET_DECL_MSG_CLASS_BASE(class_name, base_class)                                    \
  static net::MessageClassInst<class_name> messageClass;                                       \
  virtual const net::MessageClass &getMsgClass() const override final { return messageClass; } \
  class_name() : base_class {}

#define ECS_NET_DECL_MSG_CLASS(class_name) ECS_NET_DECL_MSG_CLASS_BASE(class_name, net::IMessage)

#define ECS_NET_IMPL_MSG(class_name, rout, ...)                                                       \
  net::MessageClassInst<class_name> class_name::messageClass(                                         \
    ECS_NET_MSG_CLASS_NAME(class_name), ECS_NET_MSG_CLASS_HASH(class_name), sizeof(class_name), rout, \
    (eastl::is_base_of<net::IMessageTimed, class_name>::value), ##__VA_ARGS__)
}; // namespace net

namespace ecs {
  ECS_UNICAST_EVENT_TYPE(EventNetMessage, eastl::unique_ptr<net::IMessage, net::MessageDeleter>);
}