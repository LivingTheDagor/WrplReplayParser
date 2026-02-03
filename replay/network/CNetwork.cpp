#include "network/eid.h"
#include "network/CNetwork.h"
#include "ecs/EntityManager.h"
#include "state/ParserState.h"
#include "consts.h"
#include <cassert>

std::string packet_names[]{"End", "Start", "Aircraft", "Chat", "MPI", "NextSegment", "ECS", "Snapshot", "ECS_Msg_Sync"};
namespace net
{
#define MAX_COMPRESSION_RATIO 8 // bound to avoid overcommit memory on decompression


  inline bool bitstream_decompress(const BitStream &bs, BitStream &outbs) {
    if (bs.GetReadOffset() >= bs.GetNumberOfBitsUsed())
      return false;
    assert((bs.GetReadOffset() & 7) == 0);
    assert((bs.GetNumberOfBitsUsed() & 7) == 0);
    BitSize_t compressedSize = BITS_TO_BYTES(bs.GetNumberOfUnreadBits());
    BitSize_t maxOutputSize = compressedSize * MAX_COMPRESSION_RATIO;
    outbs.ResetWritePointer();
    outbs.reserveBits(BYTES_TO_BITS(maxOutputSize));
    int readedSize = LZ4_decompress_safe((const char *) bs.GetData() + BITS_TO_BYTES(bs.GetReadOffset()),
                                         (char *) outbs.GetData(),
                                         compressedSize, maxOutputSize);
    if (readedSize <= 0)
      return false;

    outbs.SetWriteOffset(BYTES_TO_BITS(readedSize));

    return true;
  }

  void CNetwork::onPacket(ReplayPacket *pkt, int cur_time_ms) {
    BitStream & bs = pkt->stream;
    BitStream bsTemp = BitStream();
    uint8_t ptype;
    pkt->stream.Read(ptype);
    auto readCompressedIfPacketType = [&bs, &bsTemp, ptype, pkt](uint8_t ctype) -> const BitStream * {
      if (ptype != ctype)
        return &bs;
      if (bitstream_decompress(bs, bsTemp))
        return &bsTemp;
      else
      {
        EXCEPTION("failed to read compressed packet {} of {} bytes from conn #{}", ctype, BITS_TO_BYTES(bs.GetNumberOfUnreadBits()), -1);
        return nullptr;
      }
    };

    switch(ptype)
    {
      case ID_ENTITY_MSG:
      case ID_ENTITY_MSG_COMPRESSED:
      {
        ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
        if(!read_server_eid(serverEid, bs))
        {
          EXCEPTION("Failed to read the server eid from a ID_ENTITY_MSG");
        }
        ecs::EntityId eid = ecs::EntityId(serverEid);
        auto name = this->state->g_entity_mgr.getEntityTemplateName(eid);
        LOGD3("ID_ENTITY_MSG for entity {:#x} of template {}",  serverEid, name);
        // actual code
        break;
      }
      case ID_ENTITY_REPLICATION:
      case ID_ENTITY_REPLICATION_COMPRESSED:
      {
        int sentTime = 0;
        if(!bs.Read(sentTime))
        {
          EXCEPTION("Failed to read the time from a ID_ENTITY_REPLICATION");
        }

        const BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_REPLICATION_COMPRESSED);
        if (!bsToRead)
          break;
        if (!conn.readReplicationPacket(*bsToRead))
        {
          EXCEPTION("Failed to read replication packet ({}) of {} bytes", ptype, bsToRead->GetNumberOfBytesUsed());
          break;
        }
        if(this->peer)
        {
          BitStream acks{12};
          acks.Write((char)ID_ENTITY_REPLICATION);
          acks.Write(sentTime);
          conn.writeLastRecvdPacketAcks(acks);
          auto pkt = enet_packet_create(acks.GetData(), BITS_TO_BYTES(acks.GetWriteOffset()), ENET_PACKET_FLAG_UNSEQUENCED);
          enet_peer_send (peer, 0, pkt);
          LOG("sending replication ACKS");
        }
        //std::cout << "ID_ENTITY_REPLICATION\n";
        break;
        // read replication here
      }
      case ID_ENTITY_CREATION:
      case ID_ENTITY_CREATION_COMPRESSED:
      {
        const BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_CREATION_COMPRESSED);
        if (!bsToRead)
          break;

        //std::cout << "ID_ENTITY_CREATION\n";
        float cratio = 0;
        bool r = conn.readConstructionPacket(*bsToRead, cratio);
        if (!r)
          EXCEPTION("Failed to read {} construction packet of {} bytes",
                 ptype == ID_ENTITY_CREATION_COMPRESSED ? "compresssed " : "", bsToRead->GetNumberOfBytesUsed());
        break;
      }
      case ID_ENTITY_DESTRUCTION:
      {
        if(!conn.readDestructionPacket(bs))
          EXCEPTION("Failed to read destruction packet of {} bytes", bs.GetNumberOfBytesUsed());
        break;
      }
    }
  }


  CNetwork::CNetwork(ParserState *state): conn(&state->g_entity_mgr) {
    this->state = state;
  }
}

void force_link_cnet() {
  std::cout << "";
}
