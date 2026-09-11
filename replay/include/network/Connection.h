#ifndef MYEXTENSION_CONNECTION_H
#define MYEXTENSION_CONNECTION_H
extern "C" {
#include "lz4.h"
}
#include "ecs/EntityManager.h"
#include "danet/BitStream.h"
#include "utils.h"
#include "consts.h"
#include "ecs/typesAndLimits.h"
#include "ecs/componentsMap.h"
#include "BitVector.h"
#include "ecs/ComponentTypesDefs.h"
#include "ecs/ComponentPrintingImplementations.h"

#include <EASTL/vector_set.h>
#include <EASTL/vector_multiset.h>

DEFINE_HANDLE(handle_conn)
#define CONN_LOGI(format_, ...)  ELOGI(handle_conn, format_, __VA_ARGS__)
#define CONN_LOGD1(format_, ...) ELOGD1(handle_conn, format_, __VA_ARGS__)
#define CONN_LOGD2(format_, ...) ELOGD2(handle_conn, format_, __VA_ARGS__)
#define CONN_LOGD3(format_, ...) ELOGD3(handle_conn, format_, __VA_ARGS__)
#define CONN_LOGE(format_, ...)  ELOGE(handle_conn, format_, __VA_ARGS__)


struct ParserState;

namespace net {
  struct InternedStringsShared {
    std::unordered_map<std::string, uint32_t> index;
    eastl::vector<std::string> strings; // if strings[index] - "", it is not synced. strings[0] = ''
    InternedStringsShared();
  };
  struct InternedStringsRepl {
    InternedStringsShared *shared = nullptr;
    eastl::bitvector<> serialized;
    InternedStringsRepl(InternedStringsShared *shared);
  };
  // DONT ASK ABOUT THE RELIABILITY SYSTEM IM NOT TELLIN YOU NOTHIN
  typedef uint32_t ack_bits_t;
  typedef uint16_t sequence_t;
  typedef void (*packet_event_cb_t)(sequence_t seq, void *param);

  template<typename T>
  inline EA_CPP14_CONSTEXPR bool is_seq_gt(T s1, T s2) // (s1 > s2)?
  {
    const T halfMax = eastl::numeric_limits<T>::max() / 2;
    return ((s1 > s2) && (s1 - s2 <= halfMax)) || ((s2 > s1) && (s2 - s1 > halfMax));
  }

  template<typename T>
  inline EA_CPP14_CONSTEXPR bool is_seq_lt(T s1, T s2) // (s1 < s2)?
  {
    return is_seq_gt(s2, s1);
  }

  struct LessSeq : public eastl::binary_function<sequence_t, sequence_t, bool> {
    EA_CPP14_CONSTEXPR bool operator()(sequence_t a, sequence_t b) const { return is_seq_lt(a, b); }
  };

  class ReliabilitySystem {
  public:
    explicit ReliabilitySystem(sequence_t initial_seq = 0) : localSequence(initial_seq) {}

    void packetSent(int cur_time, int timeout_ms);
    void packetReceived(sequence_t seq);
    void processAck(sequence_t ack, ack_bits_t ack_bits, packet_event_cb_t packet_acked_cb,
                    packet_event_cb_t packet_lost_cb, void *cb_param, int nlost_threshold = 3);
    void update(int cur_time, packet_event_cb_t packet_lost_cb, void *cb_param);

    struct AckData {
      sequence_t rseq;
      ack_bits_t ackBits;
    };
    [[nodiscard]] AckData
    genLastSeqAckData() const; // packedReceived() is expected to be called at least once before call to this
    [[nodiscard]] sequence_t getLocalSequence() const { return localSequence; }
    void incLocalSequence() { ++localSequence; }

  private:
    sequence_t localSequence;
    struct PendingAckRec {
      sequence_t sequence;
      uint16_t nMissing;
      int timeoutAt;
      bool operator<(const PendingAckRec &rhs) const { return timeoutAt < rhs.timeoutAt; }
    };
    eastl::vector_multiset<PendingAckRec>
      pendingAcks{}; // I would stdlib, but dont feel figuring out what these actually do.
    eastl::vector_set<sequence_t, LessSeq> receivedQueue{};
  };

  class Connection {
  public:
    explicit Connection(ecs::EntityManager *mgr) : objectKeysRepl(&this->objectKeysLocal) { this->mgr = mgr; }
    ecs::EntityId deserializeConstruction(const BitStream &bs, ecs::entity_id_t serverId, uint32_t sz, float cratio);
    bool readConstructionPacket(const BitStream &bs, float compression_ratio);
    const char *deserializeTemplate(const BitStream &bs, ecs::template_t &templateId, bool &tpl_deserialized);
    bool syncReadTemplate(const BitStream &bs, ecs::template_t templateId);
    bool syncReadComponent(ecs::component_index_t serverCidx, const BitStream &bs, ecs::template_t templateId,
                           bool error);
    bool deserializeComponentConstruction(ecs::template_t server_template, const BitStream &bs,
                                          ecs::ComponentsInitializer &init, int &out_ncomp);
    bool readDestructionPacket(const BitStream &bs);
    bool readReplicationPacket(const BitStream &bs);

    bool serializeComponentReplication(ecs::EntityId eid, const ecs::ComponentRef &attr, BitStream &bs);
    bool deserializeComponentReplication(ecs::EntityId eid, const BitStream &bs);

    bool writeConstructionPacket(BitStream &bs, BitStream &tmpbs, std::vector<ecs::EntityId> &toWrite);
    void serializeConstruction(ecs::EntityId eid, BitStream &bs, bool canSkipInitial = true);
    void writeLastRecvdPacketAcks(BitStream &bs);

  public:
    mutable InternedStringsRepl objectKeysRepl;

  private:
    mutable InternedStringsShared objectKeysLocal;
    std::vector<uint8_t> construct_replication_into{}; // will hold temporary replicated data
    std::vector<std::string> serverTemplates;
    std::vector<ecs::template_t> serverToClientTemplates;
    std::vector<std::vector<ecs::component_index_t>> clientTemplatesComponents;
    std::vector<ecs::component_index_t> serverToClientCidx;
    BitVector componentsSynced{false};
    ecs::EntityManager *mgr;

    std::vector<ecs::template_t> serverTemplatesIdx; // only on server. indices of written templates
    std::vector<ecs::template_t> serverIdxToTemplates; // reverse for serverTemplatesIdx
    std::vector<uint16_t>
      serverTemplateComponentsCount; // only on server. addressed by serverTemplatesIdx[template_id].
    ecs::template_t syncedTemplate = 0;


    ReliabilitySystem reliabilitySys;
  };

} // namespace net

#endif // MYEXTENSION_CONNECTION_H
