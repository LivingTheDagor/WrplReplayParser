

#pragma once
#ifndef MYEXTENSION_CNETWORK_H
#define MYEXTENSION_CNETWORK_H
extern "C" {
#include "lz4.h"
}
#include "ecs/EntityManager.h"
#include "BitStream.h"
#include "basic_replay_structs.h"
#include "utils.h"
#include "consts.h"
#include "ecs/typesAndLimits.h"
#include "network/Connection.h"
//#include "network/message.h"
#include "enet/enet.h"

namespace net
{
  enum
  {
    ID_ENTITY_MSG = 32,
    ID_ENTITY_MSG_COMPRESSED,
    ID_ENTITY_REPLICATION, // from server - state sync, from client - acks for state sync
    ID_ENTITY_REPLICATION_COMPRESSED,
    ID_ENTITY_CREATION,
    ID_ENTITY_CREATION_COMPRESSED,
    ID_ENTITY_DESTRUCTION
  };

  class CNetwork
  {
  public:
    CNetwork(ParserState *state);

    void onPacket(ReplayPacket *pkt, int cur_time_ms);
    void setPeer(ENetPeer * peer_) {this->peer = peer_;}
    void clearPeer() {this->peer = nullptr;}
  protected:
    Connection conn;
    ParserState *state;
    ENetPeer * peer= nullptr; // only used for networking
  };
} // net

void force_link_cnet();


#endif //MYEXTENSION_CNETWORK_H
