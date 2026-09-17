

#pragma once
#ifndef MYEXTENSION_CNETWORK_H
#define MYEXTENSION_CNETWORK_H
#include "EASTL/fixed_vector.h"
extern "C" {
#include "lz4.h"
}
#include "Replay/ReplayStructs.h"
#include "utils.h"
#include "network/Connection.h"
#ifdef _ECS_CODEGEN
struct ENetPeer {};
#else
#include "enet/enet.h"
#endif
DEFINE_HANDLE(handle_cnet)
#define CNET_LOGI(format_, ...)  ELOGI(handle_cnet, format_, __VA_ARGS__)
#define CNET_LOGD1(format_, ...) ELOGD1(handle_cnet, format_, __VA_ARGS__)
#define CNET_LOGD2(format_, ...) ELOGD2(handle_cnet, format_, __VA_ARGS__)
#define CNET_LOGD3(format_, ...) ELOGD3(handle_cnet, format_, __VA_ARGS__)
#define CNET_LOGE(format_, ...)  ELOGE(handle_cnet, format_, __VA_ARGS__)

extern volatile size_t framework_network_pulls;
namespace net {
  struct MessageNetDesc;
  class IMessage;
  class Object;
  class MessageClass;
  enum {
    ID_ENTITY_MSG = 32,
    ID_ENTITY_MSG_COMPRESSED,
    ID_ENTITY_REPLICATION, // from server - state sync, from client - acks for state sync
    ID_ENTITY_REPLICATION_COMPRESSED,
    ID_ENTITY_CREATION,
    ID_ENTITY_CREATION_COMPRESSED,
    ID_ENTITY_DESTRUCTION,
    ID_NET_MSG_UNTARGETED,
    ID_NET_MSG_UNTARGETED_COMPRESSED
  };

  class CNetwork {
  public:
    CNetwork(ParserState *state);

    void onPacket(ReplayPacket *pkt, int cur_time_ms);
    void setPeer(ENetPeer *peer_) { this->peer = peer_; }
    void clearPeer() { this->peer = nullptr; }

    bool isClient() const { return true; }
    bool isServer() const { return false; }

    ecs::EntityManager &getEntityManager() const { return *conn.getEntityManager(); }

    bool sendto_untargeted(int cur_time, const IMessage &msg, Connection *receiver,
                           const MessageNetDesc *msg_net_desc = nullptr);
    Connection *getServerConnection() { return &conn; }

  protected:
    Connection conn;
    ParserState *state;
    ENetPeer *peer = nullptr; // only used for networking

    struct ObjMsg {
      const MessageClass *msgCls;
      DaNetTime rcvTime;
      BitStream bs;
      void apply(const net::Object &robj, net::Connection &from, CNetwork &cnet, const BitStream &data) const;
    };

    struct ClientWaitMsg {
      ecs::entity_id_t server_id;
      eastl::fixed_vector<ObjMsg, 1> msgs;
      bool operator<(const ClientWaitMsg &rhs) const { return server_id < rhs.server_id; }
    };
    eastl::vector_set<ClientWaitMsg> clientWaitMsgs;
  };
} // namespace net

void force_link_cnet();


#endif // MYEXTENSION_CNETWORK_H
