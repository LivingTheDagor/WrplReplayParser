

#pragma once
#ifndef MYEXTENSION_CNETWORK_H
#define MYEXTENSION_CNETWORK_H
extern "C" {
#include "lz4.h"
}
#include "ecs/EntityManager.h"
#include "danet/BitStream.h"
#include "Replay/ReplayStructs.h"
#include "utils.h"
#include "consts.h"
#include "ecs/typesAndLimits.h"
#include "network/Connection.h"
// #include "network/message.h"
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

namespace net {
  enum {
    ID_ENTITY_MSG = 32,
    ID_ENTITY_MSG_COMPRESSED,
    ID_ENTITY_REPLICATION, // from server - state sync, from client - acks for state sync
    ID_ENTITY_REPLICATION_COMPRESSED,
    ID_ENTITY_CREATION,
    ID_ENTITY_CREATION_COMPRESSED,
    ID_ENTITY_DESTRUCTION
  };

  class CNetwork {
  public:
    CNetwork(ParserState *state);

    void onPacket(ReplayPacket *pkt, int cur_time_ms);
    void setPeer(ENetPeer *peer_) { this->peer = peer_; }
    void clearPeer() { this->peer = nullptr; }

  protected:
    Connection conn;
    ParserState *state;
    ENetPeer *peer = nullptr; // only used for networking
  };
} // namespace net

void force_link_cnet();


#endif // MYEXTENSION_CNETWORK_H
