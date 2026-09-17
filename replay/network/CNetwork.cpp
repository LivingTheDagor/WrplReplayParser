#include "network/eid.h"
#include "network/CNetwork.h"
#include "ecs/EntityManager.h"
#include "state/ParserState.h"
#include "consts.h"
#include "network/message.h"
#include "network/msgDispatch.h"
#include "network/object.h"
#include "network/netEvent.h"

#include <cassert>

CREATE_HANDLE(handle_cnet, "CNetwork")
namespace net {
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
                                         (char *) outbs.GetData(), (int) compressedSize, (int) maxOutputSize);
    if (readedSize <= 0)
      return false;

    outbs.SetWriteOffset(BYTES_TO_BITS(readedSize));

    return true;
  }
  static inline bool check_routing(const IMessage &msg, const CNetwork &cnet, ecs::EntityId toEid,
                                   const net::Object *robj, bool send) {
    const MessageClass &mcls = msg.getMsgClass();
    switch (mcls.routing) {
      case ROUTING_CLIENT_TO_SERVER: return send ? cnet.isClient() : cnet.isServer();
      case ROUTING_SERVER_TO_CLIENT: return send ? cnet.isServer() : cnet.isClient();
      case ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER: {
        if (send ? cnet.isClient() : cnet.isServer()) {
          if (!robj)
            robj = cnet.getEntityManager().getNullable<net::Object>(toEid, ECS_HASH("replication"));
          G_ASSERTF(robj || (send && !cnet.getEntityManager().doesEntityExist(toEid)), "%d", (ecs::entity_id_t) toEid);
          return robj != nullptr; //&& receiver == robj->getControlledBy();
        } else
          return false;
      }
    };
    return false;
  }

  void CNetwork::ObjMsg::apply(const net::Object &robj, net::Connection &from, CNetwork &cnet,
                               const BitStream &data) const {
    ecs::EntityManager &manager = *cnet.conn.getEntityManager();
    ecs::EntityId toEid = robj.getEid();
    // G_ASSERT(!manager.isLoadingEntity(toEid));
    alignas(16) char tmpBuf[256];
    void *dataPtr = tmpBuf;
    if (DAGOR_UNLIKELY(msgCls->memSize > sizeof(tmpBuf)))
      dataPtr = cnet.state->get_allocator()->allocate(msgCls->memSize);
    // dataPtr = framemem_ptr()->alloc(msgCls->memSize);
    eastl::unique_ptr<IMessage, MessageDeleter> msg(&msgCls->create(dataPtr),
                                                    MessageDeleter(/*heap*/ false, manager.owned_by->get_allocator()));
    if (check_routing(*msg, cnet, toEid, &robj, /*send*/ false)) {
      if (msg->unpack(data, from)) {
        if (data.GetNumberOfUnreadBits() > 7) { // anymore than a byte is worrying
          std::vector<uint8_t> tmp_buff{};
          tmp_buff.resize(BITS_TO_BYTES(data.GetNumberOfUnreadBits()));
          auto unread_count = data.GetNumberOfUnreadBits();
          data.ReadBits(tmp_buff.data(), unread_count);
          auto ret = FormatHexToStream(tmp_buff);
          LOGI("network message {}/{}/{:#x} to {}<{}> has {} unread bits after unpacking; contents: {}",
               msgCls->classId, msgCls->debugClassName ? msgCls->debugClassName : "<STRIPPED>", msgCls->classHash,
               toEid, manager.getEntityTemplateName(toEid), unread_count, ret.str());
        }
        if (msgCls->flags & MF_TIMED)
          static_cast<IMessageTimed *>(msg.get())->rcvTime = rcvTime;
        msg->connection = &from;
        if (net::event::try_receive(*msg, manager, toEid))
          ;
        else {
          // Send event immediately because we need this message to be processed before futher state syncs messages
          manager.sendEventImmediate(toEid, ecs::EventNetMessage(eastl::move(msg)));
        }
      } else
        LOGE("network message {}/{}/{:#x} to {}<{}> is failed to unpack", msgCls->classId, msgCls->debugClassName,
             msgCls->classHash, (ecs::entity_id_t) toEid, manager.getEntityTemplateName(toEid));
    } else {
      static constexpr uint32_t MAX_TOTAL_LOG_COUNT = 500;
      static uint32_t totalLogCount = 0;
      if (totalLogCount < MAX_TOTAL_LOG_COUNT) {
        ++totalLogCount;
        msg->unpack(data, from);
        msg->connection = &from;
        const eastl::string msgStr = msgCls->formatMsgStr(msg.get());
        LOGE("network message {} to {}<{}> is dropped due to failed routing ({})", msgStr.c_str(),
             (ecs::entity_id_t) toEid, manager.getEntityTemplateName(toEid), (uint8_t) msgCls->routing);
      } else if (totalLogCount == MAX_TOTAL_LOG_COUNT) {
        ++totalLogCount;
        LOGE("network: too many routing failures, further warnings will be suppressed");
      }
    }
    msg.reset();
    if (dataPtr != tmpBuf)
      manager.owned_by->get_allocator()->deallocate(dataPtr, 0, 0);
  }

  static inline bool calc_parity_bit(int msgid) { return (__popcount(msgid) & 1) != 0; }

  extern ecs::EntityId msg_sink_eid;
  /*bool CNetwork::sendto_untargeted(int cur_time, const IMessage &msg, Connection *conn_,
                                   const MessageNetDesc *msg_net_desc) {
    net::Connection *conn = static_cast<net::Connection *>(conn_);
    if (!conn)
      return false;
    const MessageClass &mcls = msg.getMsgClass();
    if (MessageClass::shouldIgnoreOutgoingMessage(mcls.classId))
      return false;
    int numClassIdBits = MessageClass::getNumClassIdBits();
    G_ASSERTF(numClassIdBits >= 0, "MessageClass::init() wasn't called?");
    G_ASSERTF(mcls.classId >= 0, "%s", mcls.debugClassName);
    G_ASSERT(mcls.classId < (1 << numClassIdBits));
    BitStream bs(512);
    bs.Write((char) ID_NET_MSG_UNTARGETED);
    bs.WriteBits((uint8_t *) &mcls.classId, numClassIdBits);
    if (numClassIdBits & 7)
      bs.Write(calc_parity_bit(mcls.classId));
    bs.AlignWriteToByteBoundary();
    int headerSize = BITS_TO_BYTES(bs.GetNumberOfBitsUsed());

    msg.pack(bs);

    const net::MessageNetDesc &mdsc = msg_net_desc ? *msg_net_desc : mcls;

    BitStream bsCompressed();
    const BitStream &bsToSend = (mdsc.flags & MF_DONT_COMPRESS)
                                  ? bs
                                  : bitstream_compress(bs, headerSize, ID_NET_MSG_UNTARGETED_COMPRESSED, bsCompressed,
                                                       DEFAULT_COMPRESSION_THRESHOLD);
    // TRACE_NET_STAT(tx, mcls.debugClassName, bs, bsToSend, conn);
    PacketPriority pprio = (mdsc.flags & MF_URGENT) ? SYSTEM_PRIORITY : MEDIUM_PRIORITY;
    return conn->send(cur_time, bsToSend, pprio, mdsc.reliability, mdsc.channel, mdsc.dupDelay);
  }*/

  void CNetwork::flushClientWaitMsgs(ecs::entity_id_t serverEid) {
    G_ASSERT(isClient());
    // G_ASSERT(serverConnection);
    auto it = clientWaitMsgs.find(ClientWaitMsg{serverEid});
    if (it == clientWaitMsgs.end())
      return;
    if (const Object *robj = Object::getByEid(ecs::EntityId(serverEid), &this->getEntityManager())) {
      G_ASSERT(!it->msgs.empty());
      for (auto &msg: it->msgs)
        msg.apply(*robj, conn, *this, msg.bs);
    } else
      G_ASSERTF(0, "Failed to resolve serverEid %d or replication component", serverEid);
    clientWaitMsgs.erase(it);
  }

  void CNetwork::onPacket(ReplayPacket *pkt, int cur_time_ms) {
    BitStream &bs = pkt->stream;
    BitStream bsTemp = BitStream();
    uint8_t ptype;
    pkt->stream.Read(ptype);
    auto readCompressedIfPacketType = [&bs, &bsTemp, ptype, pkt](uint8_t ctype) -> const BitStream * {
      if (ptype != ctype)
        return &bs;
      if (bitstream_decompress(bs, bsTemp))
        return &bsTemp;
      else {
        EXCEPTION("failed to read compressed packet {} of {} bytes from conn #{}", ctype,
                  BITS_TO_BYTES(bs.GetNumberOfUnreadBits()), -1);
      }
    };

    switch (ptype) {
      case ID_ENTITY_MSG:
      case ID_ENTITY_MSG_COMPRESSED: {
        ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
        if (!net::read_server_eid(serverEid, bs)) {
          CNET_LOGE("Failed to read server eid from conn, incompatible network protocol?");
          break;
        }
        ecs::EntityId eid = ecs::EntityId(serverEid);
        CNET_LOGD3("ID_ENTITY_MSG for entity {:#x} of template {}", serverEid,
                   this->state->g_entity_mgr.getEntityTemplateName(eid));
        int msgId = 0;
        int nClsIdBits = MessageClass::getNumClassIdBits();
        if (!bs.ReadBits((uint8_t *) &msgId, nClsIdBits))
          msgId = -1;
        else if (nClsIdBits & 7) {
          bool parityBit = false;
          if (!bs.Read(parityBit))
            msgId = -1;
          else if (parityBit != calc_parity_bit(msgId))
            msgId = -2;
        }
        if (!MessageClass::validateIncomingMessage(msgId, 0))
          break;
        const MessageClass *msgCls = MessageClass::getById(msgId);
        if (!msgCls) {
          CNET_LOGI("ID_ENTITY_MSG: msgId {} for eid {}", msgId, eid);
          break;
        }
        bs.AlignReadToByteBoundary();
        const BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_MSG_COMPRESSED);
        if (!bsToRead)
          break;
        // TRACE_NET_STAT(rx, msgCls->debugClassName, *bsToRead, bs, conn);
        ObjMsg omsg;
        omsg.msgCls = msgCls;
        if (msgCls->flags & MF_TIMED)
          omsg.rcvTime = pkt->timestamp_ms;
        if (const Object *robj = Object::getByEid(eid, &this->state->g_entity_mgr))
          omsg.apply(*robj, conn, *this, *bsToRead);
        else if (true /*isClient()*/) {
          if ((msgCls->flags & MF_DISCARD_IF_NO_ENTITY) != 0)
            ; // do nothing (i.e. discard)
          else {
            auto it = clientWaitMsgs.insert(ClientWaitMsg{serverEid}).first;
            it->msgs.push_back(eastl::move(omsg));
            it->msgs.back().bs = *bsToRead; // copy
          }
        } else
          CNET_LOGE("Can't resolve network entity by eid {} with msg = {}/{:#x}. Message will be dropped.", serverEid,
                    msgCls->debugClassName, msgCls->classHash);
      } break;
      case ID_NET_MSG_UNTARGETED:
      case ID_NET_MSG_UNTARGETED_COMPRESSED: {
        // GET_CONN_OR_LEAVE(pkt->systemIndex);
        int msgId = 0;
        int nClsIdBits = MessageClass::getNumClassIdBits();
        if (!bs.ReadBits((uint8_t *) &msgId, nClsIdBits))
          msgId = -1;
        else if (nClsIdBits & 7) {
          bool parityBit = false;
          if (!bs.Read(parityBit))
            msgId = -1;
          else if (parityBit != calc_parity_bit(msgId))
            msgId = -2;
        }
        if (!MessageClass::validateIncomingMessage(msgId, 0))
          break;
        CNET_LOGI("ID_ENTITY_MSG_UNTARGETED: msgId {}", msgId);
        const MessageClass *msgCls = MessageClass::getById(msgId);
        if (!msgCls)
          break;
        bs.AlignReadToByteBoundary();
        const BitStream *bsToRead = readCompressedIfPacketType(ID_NET_MSG_UNTARGETED_COMPRESSED);
        if (!bsToRead)
          break;
        // TRACE_NET_STAT(rx, msgCls->debugClassName, *bsToRead, bs, conn);
        alignas(16) char tmpBuf[256];
        void *dataPtr = tmpBuf;
        if (DAGOR_UNLIKELY(msgCls->memSize > sizeof(tmpBuf)))
          dataPtr = state->get_allocator()->allocate(msgCls->memSize);
        // dataPtr = framemem_ptr()->alloc(msgCls->memSize);
        eastl::unique_ptr<IMessage, MessageDeleter> msg(
          &msgCls->create(dataPtr), MessageDeleter(/*heap*/ false, this->getEntityManager().owned_by->get_allocator()));
        if (msg->unpack(*bsToRead, conn)) {
          if (msgCls->flags & MF_TIMED)
            static_cast<IMessageTimed *>(msg.get())->rcvTime = pkt->timestamp_ms;
          msg->connection = &conn;
          if (!net::dispatch_net_msg_handler(msg.get()))
            CNET_LOGE("Untargeted net msg {}/{:#x} has no handler - dropped", msgCls->debugClassName,
                      msgCls->classHash);
        } else
          CNET_LOGE("Untargeted net msg {}/{:#x} failed to unpack", msgCls->debugClassName, msgCls->classHash);
        msg.reset();
        if (dataPtr != tmpBuf)
          state->get_allocator()->deallocate(dataPtr, 0, 0);
        // framemem_ptr()->free(dataPtr);
      } break;
      case ID_ENTITY_REPLICATION:
      case ID_ENTITY_REPLICATION_COMPRESSED: {
        int sentTime = 0;
        if (!bs.Read(sentTime)) {
          EXCEPTION("Failed to read the time from a ID_ENTITY_REPLICATION");
        }

        const BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_REPLICATION_COMPRESSED);
        if (!bsToRead)
          break;
        if (!conn.readReplicationPacket(*bsToRead)) {
          EXCEPTION("Failed to read replication packet ({}) of {} bytes", ptype, bsToRead->GetNumberOfBytesUsed());
          break;
        }
        if (this->peer) {
          BitStream acks{12};
          acks.Write((char) ID_ENTITY_REPLICATION);
          acks.Write(sentTime);
          conn.writeLastRecvdPacketAcks(acks);
          auto pkt =
            enet_packet_create(acks.GetData(), BITS_TO_BYTES(acks.GetWriteOffset()), ENET_PACKET_FLAG_UNSEQUENCED);
          enet_peer_send(peer, 0, pkt);
          CNET_LOGD2("sending replication ACKS");
        }
        // std::cout << "ID_ENTITY_REPLICATION\n";
        break;
        // read replication here
      }
      case ID_ENTITY_CREATION:
      case ID_ENTITY_CREATION_COMPRESSED: {
        const BitStream *bsToRead = readCompressedIfPacketType(ID_ENTITY_CREATION_COMPRESSED);
        if (!bsToRead)
          break;
        auto constructCb = [this](Connection &conn, ecs::entity_id_t serverEid) {
          // if (&conn != serverConnection.get()) // Note: in theory different instance might get reallocated to the
          // same exact address as
          //   // old one
          //     return;
          // conn.applyDelayedAttrsUpdate(serverEid);
          flushClientWaitMsgs(serverEid);
        };
        // std::cout << "ID_ENTITY_CREATION\n";
        float cratio = 0;
        bool r = conn.readConstructionPacket(*bsToRead, cratio, constructCb);
        if (!r)
          EXCEPTION("Failed to read {} construction packet of {} bytes",
                    ptype == ID_ENTITY_CREATION_COMPRESSED ? "compresssed " : "", bsToRead->GetNumberOfBytesUsed());
        break;
      }
      case ID_ENTITY_DESTRUCTION: {
        if (!conn.readDestructionPacket(bs))
          EXCEPTION("Failed to read destruction packet of {} bytes", bs.GetNumberOfBytesUsed());
        break;
      }
    }
  }


  CNetwork::CNetwork(ParserState *state) : conn(&state->g_entity_mgr) { this->state = state; }
} // namespace net

void force_link_cnet() { std::cout << ""; }
