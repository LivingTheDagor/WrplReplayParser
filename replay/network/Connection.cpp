#include "network/Connection.h"
#include "network/eid.h"
#include "ecs/BitStreamDeserializer.h"
#include "ecs/EntityManager.h"
#include "network/Object.h"

#define REPL_VER(x) \
  if (!(x))         \
    return failRet

namespace net
{

#define SEQ_HISTORY_DEPTH 1024

  void ReliabilitySystem::packetSent(int cur_time, int timeout_ms)
  {
#if DAGOR_DBGLEVEL > 0
    bool alreadyExist = eastl::find_if(pendingAcks.begin(), pendingAcks.end(),
                        [this](const PendingAckRec &qr) { return qr.sequence == localSequence; }) != pendingAcks.end();
  G_ASSERT(!alreadyExist);
#endif
    pendingAcks.insert(PendingAckRec{localSequence, 0, cur_time + timeout_ms});
    incLocalSequence();
  }

  void ReliabilitySystem::packetReceived(sequence_t seq)
  {
    receivedQueue.insert(seq);
  }



  template <typename T>
  inline T seq_diff(T big, T small)
  {
    if (big >= small)
      return big - small;
    else // overflow
      return (T)((eastl::numeric_limits<sequence_t>::max() - small) + big + 1);
  }

  void ReliabilitySystem::processAck(sequence_t ack, ack_bits_t ack_bits, packet_event_cb_t packet_acked_cb,
                                     packet_event_cb_t packet_lost_cb, void *cb_param, int nlost_threshold)
  {
        G_FAST_ASSERT(packet_acked_cb != NULL && packet_lost_cb != NULL);
    sequence_t prevAck = ack - 1;
    for (auto it = pendingAcks.begin(); it != pendingAcks.end();)
    {
      packet_event_cb_t cb = NULL;
      if (!is_seq_gt(it->sequence, ack)) // seq <= ack
      {
        if (it->sequence == ack)
          cb = packet_acked_cb;
        else
        {
          int bitIdx = seq_diff(prevAck, it->sequence);
          if (bitIdx < sizeof(ack_bits) * CHAR_BIT)
          {
            cb = (ack_bits & (1 << bitIdx)) ? packet_acked_cb : NULL;
            if (!cb && ++it->nMissing >= nlost_threshold)
              cb = packet_lost_cb;
          }
          // else To consider: mark all packets > this number of packets (32+1) as lost
        }
      }
      if (cb)
      {
        cb(it->sequence, cb_param);
        it = pendingAcks.erase(it);
      }
      else
        ++it;
    }
  }

  bool Connection::readConstructionPacket(const BitStream &bs, float compression_ratio) {
    auto failRet = false;

    uint8_t written = 0;
    REPL_VER(bs.Read(written));
    for (int i = 0, n = written + 1; i < n; ++i)
    {
      ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
      REPL_VER(read_server_eid(serverEid, bs));
      ecs::EntityId resolvedEid(serverEid);
      uint32_t blockSizeBytes = 0;
      REPL_VER(bs.ReadCompressed(blockSizeBytes));
      G_ASSERT(blockSizeBytes > 0);
      BitSize_t startPos = bs.GetReadOffset();
      if (DAGOR_LIKELY(mgr->getEntityTemplateId(resolvedEid) == ecs::INVALID_TEMPLATE_INDEX))
      {
        ecs::EntityId eid = deserializeConstruction(bs, serverEid, blockSizeBytes, compression_ratio);

        if (!eid)
          EXCEPTION("Construction of entity of eid {:#x} failed", serverEid);
      }
      else
        EXCEPTION("Attempt to create already existing network entity {}<{}>", serverEid, mgr->getEntityTemplateName(resolvedEid));
      bs.SetReadOffset(startPos + BYTES_TO_BITS(blockSizeBytes)); // generally this isnt actually needed
    }
    return true;
  }


  void ReliabilitySystem::update(int cur_time, packet_event_cb_t packet_lost_cb, void *cb_param)
  {
    // check timeout for pendings acks
    for (auto it = pendingAcks.begin(); it != pendingAcks.end();)
    {
      if (cur_time >= it->timeoutAt)
      {
        packet_lost_cb(it->sequence, cb_param);
        it = pendingAcks.erase(it);
      }
      else
        break;
    }

    // keep only sizeof(ack_bits_t)*8+1 last received entries
    if (!receivedQueue.empty())
    {
      sequence_t numSeqToKeep = SEQ_HISTORY_DEPTH + 1;
      sequence_t minSeq = receivedQueue.back() - numSeqToKeep;
      auto it = receivedQueue.begin();
      while (it != receivedQueue.end() && !is_seq_gt(*it, minSeq))
        it = receivedQueue.erase(it);
    }
  }

  ReliabilitySystem::AckData ReliabilitySystem::genLastSeqAckData() const
  {
    G_ASSERT(!receivedQueue.empty()); // this method is expected to be called after 'packetReceived'

    ack_bits_t ackBits = 0;
    sequence_t remoteSequence = receivedQueue.back(), prevRemoteSeq = remoteSequence - 1;
    for (int i = int(receivedQueue.size()) - 2; i >= 0; --i)
    {
      int bitIdx = seq_diff(prevRemoteSeq, receivedQueue[i]);
      if (bitIdx < sizeof(ackBits) * CHAR_BIT)
        ackBits |= 1 << bitIdx;
      else
        break;
    }

    return AckData{remoteSequence, ackBits};
  }

  ecs::EntityId
  Connection::deserializeConstruction(const BitStream &bs, ecs::entity_id_t serverId, uint32_t sz, float cratio) {
    G_ASSERT(serverId != ecs::ECS_INVALID_ENTITY_ID_VAL);
    G_UNUSED(sz);
    G_UNUSED(cratio);
    ecs::template_t serverTemplate = ecs::INVALID_TEMPLATE_INDEX;
    bool templDeserialized = false;
    const char *templName = deserializeTemplate(bs, serverTemplate, templDeserialized);
    if (!templName)
    {
      EXCEPTION("Failed to deserialize template for server entity <%d>", serverId);
      return ecs::INVALID_ENTITY_ID;
    }
    //std::cout << "constructing entity of template: " << templName << "\n";

    int ncomp = 0;
    ecs::ComponentsInitializer ainit;
    ainit.reserve(clientTemplatesComponents[serverTemplate].size());
    //LOG("deserializing for entity {:#x} of template {}", serverId, templName);

    if (!deserializeComponentConstruction(serverTemplate, bs, ainit, ncomp))
      EXCEPTION("Failed to deserializeComponentConstruction");

    ecs::EntityId srvEid(serverId);
    //ecs::ComponentTypeInfo<ecs::EntityId>::typed
    ainit[ECS_HASH("eid")] = ecs::Component(srvEid);
    mgr->createEntity(srvEid, this->serverToClientTemplates[serverTemplate], std::move(ainit));
    return srvEid;
  }


  const char *Connection::deserializeTemplate(const BitStream &bs, ecs::template_t &templateId, bool &tpl_deserialized)
  {
    // it is too expensive to always serialize template name, so we just rely on construction being reliable ordered, and write name
    // once.
    templateId = ecs::INVALID_TEMPLATE_INDEX;
    if (!bs.ReadCompressed(templateId))
      return nullptr;
    if (templateId >= serverTemplates.size())
    {
      if (!syncReadTemplate(bs, templateId))
        return nullptr;
      tpl_deserialized = true;
    }
    G_ASSERT(serverTemplates.size() > templateId && !serverTemplates[templateId].empty());
    return serverTemplates[templateId].c_str();
  }

  static bool read_component_index(ecs::component_index_t &cidx, const BitStream &bs)
  {
    return bs.ReadCompressed(cidx); //==todo: we only need 12 bits really. Write compressed form of.
  }

  bool Connection::syncReadTemplate(const BitStream &bs, ecs::template_t templateId)
  {
    G_ASSERT(templateId == serverTemplates.size()); // We rely on templateId being monotonically increased counter here

    serverTemplates.resize(templateId + 1);
    serverToClientTemplates.resize(templateId+1);
    clientTemplatesComponents.resize(templateId + 1);

    if (!bs.Read(serverTemplates[templateId])) // ref to template
      return false;
    LOGD2("Parsing new template: {}", serverTemplates[templateId].c_str());

    ecs::template_t templId = mgr->buildTemplateIdByName(serverTemplates[templateId].c_str());
    if (templId != ecs::INVALID_TEMPLATE_INDEX)
      mgr->instantiateTemplate(templId);
    else
    {
      // todo: create this template instead! we know everything from server side!
      EXCEPTION("template <{}> is not in database, and so can't be created", serverTemplates[templateId].c_str());
    }
    serverToClientTemplates[templateId] = templId;

    uint16_t componentsInTemplate = 0;
    if (!bs.Read(componentsInTemplate))
      return false;
    clientTemplatesComponents[templateId].resize(componentsInTemplate);
    for (uint16_t cid = 0; cid != componentsInTemplate; ++cid)
    {
      ecs::component_index_t serverCidx = 0;
      if (!read_component_index(serverCidx, bs))
        return false;
      G_ASSERT(serverCidx != ecs::INVALID_COMPONENT_INDEX);
      ecs::component_index_t cliCidx =
          serverCidx < serverToClientCidx.size() ? serverToClientCidx[serverCidx] : ecs::INVALID_COMPONENT_INDEX;
      if (cliCidx != ecs::INVALID_COMPONENT_INDEX)
      {
        G_ASSERT(componentsSynced.test(serverCidx, false));
        clientTemplatesComponents[templateId][cid] = cliCidx;
      }
      else if (!componentsSynced.test(serverCidx, false))
      {
        if (!syncReadComponent(serverCidx, bs, templateId, false))
          return false;
        clientTemplatesComponents[templateId][cid] = serverToClientCidx[serverCidx];
      }
    }
    return true;
  }

  bool Connection::syncReadComponent(ecs::component_index_t serverCidx, const BitStream &bs, ecs::template_t templateId,
                                     bool error)
  {
    G_UNUSED(templateId);
    // 8 bytes. However, can write only two bytes, if we preserve component table (which we should)
    ecs::component_t name = 0;
    ecs::component_type_t type = 0;
    if (!(bs.Read(name) && bs.Read(type)))
      return false;
    ecs::component_index_t clientCidx = ecs::g_ecs_data->getDataComponents()->getIndex(name); // try to immediately resolve component
    if (clientCidx == ecs::INVALID_COMPONENT_INDEX)                                              // component is missing on client
    {
      const ecs::type_index_t typeIdx = ecs::g_ecs_data->getComponentTypes()->findType(type);
      if (error)
      {
        //int loglev = typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX ? LOGLEVEL_WARN : LOGLEVEL_ERR;
        //G_UNUSED(loglev);
        EXCEPTION("component scidx={}, name={:#x} type={:#x}({}) is missing in template <{}> on client", serverCidx, name, type,
                   ecs::g_ecs_data->getComponentTypes()->getName(typeIdx), serverTemplates[templateId].c_str());
      }
      if (typeIdx != ecs::INVALID_COMPONENT_TYPE_INDEX)
        clientCidx =
            ecs::g_ecs_data->createComponent(HashedConstString{nullptr, name}, typeIdx, nullptr);
      else
        EXCEPTION("Unknown Component found while parsing template. hash: {:#x}", type);
    }
    if (serverCidx >= serverToClientCidx.size())
      serverToClientCidx.resize(serverCidx + 1, ecs::INVALID_COMPONENT_INDEX);
    serverToClientCidx[serverCidx] = clientCidx;
    componentsSynced.set(serverCidx, true);
    return true;
  }
  bool Connection::deserializeComponentConstruction(ecs::template_t server_template, const BitStream &bs,
                                                    ecs::ComponentsInitializer &init, int &out_ncomp)
  {
    BitstreamDeserializer deserializer(bs, mgr, &objectKeys);
    uint16_t compCount = 0;
    const uint16_t templateComponentsCount = (uint16_t)clientTemplatesComponents[server_template].size();
    if (templateComponentsCount < 256)
    {
      uint8_t compCount8 = 0;
      if (!bs.Read(compCount8))
        return false;
      compCount = compCount8;
    }
    else if (!bs.Read(compCount))
      return false;
    for (uint16_t comp = 0, i = 0; i < compCount; ++i)
    {
      uint16_t ofs;
      if (templateComponentsCount < 256)
      {
        uint8_t ofs8;
        if (!bs.Read(ofs8))
          return false;
        ofs = ofs8;
      }
      else if (!bs.ReadCompressed(ofs))
        return false;
      comp = (i == 0) ? ofs : (uint16_t)(comp + ofs + 1);
      //std::cout << comp << " : " << bs.GetReadOffset() << "\n";
      if (comp >= templateComponentsCount)
      {
        EXCEPTION("Invalid template component index {} for template local idx {}<{}> (count {})", comp, server_template,
               serverTemplates[server_template].c_str(), templateComponentsCount);
        return false;
      }

      ecs::component_index_t cidx = clientTemplatesComponents[server_template][comp];
      if (cidx == ecs::INVALID_COMPONENT_INDEX)
        return false;
      auto cmp = ecs::g_ecs_data->getDataComponents()->getDataComponent(cidx);
      ecs::component_type_t componentTypeName = cmp->componentHash;
      ecs::component_t componentNameHash = cmp->hash;

      //std::cout << cmp->getName().data() << "("<< ecs::g_ecs_data->getComponentTypes()->getComponentData(cmp->componentIndex)->name.data() << ")\n";
      //std::cout.flush();
      if (ecs::MaybeComponent mbcomp = deserialize_init_component_typeless(componentTypeName, cidx, deserializer, mgr))
      {
        if (mbcomp.has_value())
          init[HashedConstString({"!net_replicated!", componentNameHash})] = std::move(*mbcomp);
      }
      else
        return false;
    }
    out_ncomp = compCount;
    return true;
  }

  bool Connection::readDestructionPacket(const BitStream &bs) {
    uint8_t written = 0;
    const bool failRet = false;
    REPL_VER(bs.Read(written));
    for (int i = 0, n = written + 1; i < n; ++i)
    {
      ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
      REPL_VER(read_server_eid(serverEid, bs));
      const ecs::EntityId resolvedEid(serverEid);
      if(DAGOR_LIKELY(mgr->doesEntityExist(resolvedEid)))
      {
        mgr->destroyEntity(resolvedEid);
      }
      else
      {
        EXCEPTION("Entity {:#x} already destroyed?", serverEid);
      }
    }
    return true;
  }

  template <typename F>
  static inline uint32_t write_block(BitStream &bs, BitStream &tmpbs, const F &writefn)
  {
    memset(tmpbs.GetData(), 0, tmpbs.GetNumberOfBytesUsed());
    tmpbs.ResetWritePointer();

    writefn(tmpbs);

    uint32_t blockSizeBytes = tmpbs.GetNumberOfBytesUsed();
    if (blockSizeBytes)
    {
      bs.WriteCompressed(blockSizeBytes);
      bs.Write((const char *)tmpbs.GetData(), blockSizeBytes);
    }

    return blockSizeBytes;
  }

  static void write_component_index(ecs::component_index_t cidx, BitStream &bs)
  {
    bs.WriteCompressed(cidx); //==todo: we only need 12 bits really. Write compressed form of.
  }

  // bs is where data is actually stored, tmpbs is where temporary data is stored.
  bool Connection::writeConstructionPacket(BitStream &bs, BitStream &tmpbs, std::vector<ecs::EntityId> &toWrite)
  {
    BitSize_t afterHdrPos = bs.GetWriteOffset();
    int written = 0;
    bs.Write((uint8_t)0);

    for (auto & eid : toWrite)

    {
      net::write_server_eid((ecs::entity_id_t)eid, bs);

      uint32_t blockSizeBytes = write_block(bs, tmpbs, [this, eid](BitStream &bs2) { serializeConstruction(eid, bs2); });

      G_FAST_ASSERT(blockSizeBytes);
      G_UNUSED(blockSizeBytes);
#if DAECS_EXTENSIVE_CHECKS
      if (blockSizeBytes >= (1 << 14))
      logwarn("Construction packet of entity %d(%s) is %dK!", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid),
        blockSizeBytes >> 10);
#endif



      if (++written > UCHAR_MAX)
        break;
    }

    if (written)
    {
      bs.WriteAt(uint8_t(written - 1), afterHdrPos);
      return true;
    }
    else
      return false;
  }
  bool should_replicate(ecs::ComponentRef ref)
  {
    auto data_comp = ecs::g_ecs_data->getDataComponents()->getDataComponent(ref.getComponentId());
    auto name = data_comp->getName();
    auto comp = ecs::g_ecs_data->getComponentTypes()->getComponentData(data_comp->componentIndex);

    if(name == "replication" || name == "eid" || comp->name == "ecs::Tag")
      return false;
    return true;
  }

  void Connection::serializeConstruction(ecs::EntityId eid, BitStream &bs, bool canSkipInitial) {
#if NET_STAT_PROFILE_INITIAL_SIZES
    BitSize_t beginWr = bs.GetWriteOffset();
#endif
    const int componentsCount = mgr->getNumComponents(eid);
    const ecs::template_t templateId = mgr->getEntityTemplateId(eid);

    //Object *object = Object::getByEid(eid);
    //ObjectReplica *replica = getReplicaByEid(eid);

    auto iterateReplicatable = [&, componentsCount](auto fn) {
      for (ecs::archetype_component_id cid = 0; cid < componentsCount; ++cid) {
        auto comp = mgr->getComponentRef(eid, cid);
        if (should_replicate(comp)) // never written
          fn(comp, cid);
      }
    };
    ecs::template_t serverWrittenIdx =
        templateId < serverTemplatesIdx.size() ? serverTemplatesIdx[templateId] : ecs::INVALID_TEMPLATE_INDEX;
    if (serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX)
      bs.WriteCompressed(serverWrittenIdx); // ref to template
    else                                    // not yet synced/known to client
    {
      // it is too expensive to always serialize template name, so we just rely on construction being reliable ordered, and write name
      // once.
      if (serverTemplatesIdx.size() <= templateId)
        serverTemplatesIdx.resize(templateId + 1, ecs::INVALID_TEMPLATE_INDEX);
      serverWrittenIdx = syncedTemplate++;
      serverIdxToTemplates.resize(syncedTemplate);
          G_FAST_ASSERT(serverWrittenIdx != ecs::INVALID_TEMPLATE_INDEX);
      serverTemplatesIdx[templateId] = serverWrittenIdx;
      serverIdxToTemplates[serverWrittenIdx] = templateId;
      bs.WriteCompressed(serverWrittenIdx); // ref to template
      {

        bs.Write(ecs::g_ecs_data->getTemplateDB()->getTemplate(templateId)->getName()); // _local -> _remote
      }
      // todo: actually, we'd better serialize template, if it is different on server and client. Would require some state to check.
      const BitSize_t blockSizePos = bs.GetWriteOffset();
      uint16_t componentsInTemplate = 0;
      bs.Write(componentsInTemplate);
      iterateReplicatable([&](ecs::ComponentRef comp, uint16_t) {
        componentsInTemplate++;
        const ecs::component_index_t cidx = comp.getComponentId();
        write_component_index(cidx, bs);
        if (!componentsSynced.test(cidx, false)) {
          // 8 bytes, if component is first time synced
          bs.Write(ecs::g_ecs_data->getDataComponents()->getDataComponent(cidx)->hash);
          bs.Write(comp.getUserType());
          //LOG("Writing %s to template\n", mgr->getDataComponents()->getDataComponent(cidx)->getName().data());
          componentsSynced.set(cidx, true);
        }
      });
      bs.WriteAt(componentsInTemplate, blockSizePos);
      if (serverWrittenIdx >= serverTemplateComponentsCount.size())
        serverTemplateComponentsCount.resize(serverWrittenIdx + 1);
      serverTemplateComponentsCount[serverWrittenIdx] = componentsInTemplate;
    }

    //const ecs::component_index_t *ignoredComponents = get_template_ignored_initial_components(templateId), *ignoredComponentsE = nullptr;
    //if (ignoredComponents)
    //{
    //  size_t nComp = *ignoredComponents++;
    //  ignoredComponentsE = ignoredComponents + nComp;
    //}

    BitstreamSerializer serializer(*mgr, bs, &objectKeys);
    const BitSize_t countSizePos = bs.GetWriteOffset();
    const bool lessThan256 = serverTemplateComponentsCount[serverWrittenIdx] < 256;
    uint16_t componentsInTemplate = 0, prevComponent = 0, writtenComponents = 0;
    if (lessThan256)
      bs.Write(uint8_t(writtenComponents));
    else
      bs.Write(writtenComponents);
    iterateReplicatable([&, eid](ecs::ComponentRef comp, uint16_t cid) {
#if NET_STAT_PROFILE_INITIAL_SIZES
      auto beginWrComp = serializer.bs.GetWriteOffset();
#endif
      if (false)
      {
        // skip
      }
      else
      {
        // first one is not diff
        const uint16_t ofs = (writtenComponents++ == 0) ? componentsInTemplate : uint16_t(componentsInTemplate - prevComponent - 1);
        if (lessThan256) // write one byte
        {
              G_FAST_ASSERT(ofs <= UCHAR_MAX);
          serializer.bs.Write(uint8_t(ofs));
        }
        else
          serializer.bs.WriteCompressed(ofs); // write compressed ofs
        //LOG("serializing %s at %i\n", ecs::g_entity_mgr->getDataComponents()->getName(comp.getComponentId()).data(), bs.GetWriteOffset());
        ecs::serialize_entity_component_ref_typeless(comp, serializer, mgr);
        prevComponent = componentsInTemplate;
      }
      componentsInTemplate++;
#if NET_STAT_PROFILE_INITIAL_SIZES
      const uint32_t bits = serializer.bs.GetWriteOffset() - beginWrComp;
    if (bits && !isBlackHole())
      templatesComponentSize[templateId | (comp.getComponentId() << 16)] += bits;
#endif
    });
    G_ASSERT(lessThan256 == (componentsInTemplate < 256));

    // Remove this check when we sure enough that it's not happens and use bitmap for storing info about count < 256 instead
    if (DAGOR_UNLIKELY(componentsInTemplate != serverTemplateComponentsCount[serverWrittenIdx]))
      LOGE("Inconsistent replication components count %d (cur) != %d (initial) in template %d<%s>", componentsInTemplate,
             serverTemplateComponentsCount[serverWrittenIdx], templateId, mgr->getEntityTemplateName(eid));

    if (lessThan256)
      bs.WriteAt(uint8_t(writtenComponents), countSizePos);
    else
      bs.WriteAt(uint16_t(writtenComponents), countSizePos);
#if NET_STAT_PROFILE_INITIAL_SIZES
    if (!isBlackHole())
    templatesSize[templateId] += bs.GetWriteOffset() - beginWr;
#endif
  }

  typedef uint16_t sequence_t;
  bool Connection::readReplicationPacket(const BitStream &bs)
  {

    constexpr bool failRet = false;
    sequence_t remoteSequence = sequence_t(-1);
    REPL_VER(bs.Read(remoteSequence));
    //LOG("squence_t: %i\n", remoteSequence);
    do
    {
      bool exists = false;
      REPL_VER(bs.Read(exists));
      if (!exists)
        break;
      ecs::entity_id_t serverEid = ecs::ECS_INVALID_ENTITY_ID_VAL;
      REPL_VER(read_server_eid(serverEid, bs));
      uint32_t blockSizeBytes = 0;
      REPL_VER(bs.ReadCompressed(blockSizeBytes));
      G_ASSERT(blockSizeBytes > 0);
      BitSize_t startPos = bs.GetReadOffset();

      Object *nobj = Object::getByEid(ecs::EntityId(serverEid), mgr);
      if (nobj)
      {
        REPL_VER(nobj->deserializeComps(bs, this));
        G_ASSERT(BITS_TO_BYTES(bs.GetReadOffset() - startPos) == blockSizeBytes);
      }
      else // attrs update for missing local object (assume that creation packet is not received yet, and save it for future apply)
      {
        EXCEPTION("failed to get net::Object from stream");
        // this code only matters if we get
        //auto it = delayedAttrsUpdate.insert(DelayedAttrsUpdateRec{serverEid}).first;
        //SmallTab<uint8_t, MidmemAlloc> &tab = it->data.emplace_back();
        //clear_and_resize(tab, blockSizeBytes);
        //G_VERIFY(bs.ReadBits(&tab[0], BYTES_TO_BITS(blockSizeBytes)));
      }
      bs.SetReadOffset(startPos + BYTES_TO_BITS(blockSizeBytes)); // rewind to next block
    } while (1);

    reliabilitySys.packetReceived(remoteSequence);

    return true;
  }

  bool Connection::deserializeComponentReplication(ecs::EntityId eid, const BitStream &bs) {

    ecs::component_index_t serverCidx = 0;
    if (!read_component_index(serverCidx, bs))
      return false;

    G_ASSERT(componentsSynced.test(serverCidx, false)); // should never happen, no need for sanity check in release
    const ecs::component_index_t clientCidx = serverToClientCidx[serverCidx];
    if(eid.get_handle() == 0x1c00a65 && ecs::g_ecs_data->getDataComponents()->getDataComponent(clientCidx)->hash == 0x96B0BD40)
      LOG("WOMP");
    if (clientCidx == ecs::INVALID_COMPONENT_INDEX) // we can't deserialize it, which means type was unknown!
      return false;
    auto datacomp = ecs::g_ecs_data->getDataComponents()->getDataComponent(clientCidx);

    BitSize_t beforeReadPos = bs.GetReadOffset();
    BitstreamDeserializer bsds(bs, mgr, &objectKeys);
    ecs::ComponentRef cref = mgr->getComponentRefCidx(eid, clientCidx);
    bool crefIsNull = cref.isNull();

    if (DAGOR_LIKELY(!crefIsNull && deserialize_component_typeless(cref, bsds, *mgr)))
    {
      LOGD3("Replicating Component {}({}) for entity {:#x} for template {}. data: {}",
          datacomp->getName(),
          ecs::g_ecs_data->getComponentTypes()->getName(cref.getTypeId()),
          eid.get_handle(),
          this->mgr->getEntityTemplateName(eid), cref.toString(nullptr));
      //replicated_component_on_client_deserialize(eid, clientCidx);
      return true;
    }
    const char *warn_type;
    if(crefIsNull) {
      // the warning is because this entity does not have that component
      warn_type = "Unknown/missing component";
    }
    else {
      // the warning is because we failed to deserialize the component
      warn_type = "Failed to deserialize component";
    }
    bs.SetReadOffset(beforeReadPos);
    auto comp = ecs::g_ecs_data->getComponentTypes()->getComponentData(datacomp->componentIndex);
    LOGE("{} {}<{:#x}> of type {}<{:#x}> for entity {:#x}<{}>", warn_type,
         datacomp->getName(), datacomp->hash,
         comp->name, comp->hash,
         eid.get_handle(),
         this->mgr->getEntityTemplateName(eid));
    return bsds.skip(clientCidx, *datacomp);
  }

  bool Connection::serializeComponentReplication(ecs::EntityId eid, const ecs::ComponentRef &comp, BitStream &bs)
  {
    if (!componentsSynced.test(comp.getComponentId(), false)) // FIXME: race on components replication & re-creation
    {
      G_UNUSED(eid);
      //EXCEPTION("");
      LOGE("Attempt to serialize not-yet synced component <{}> of type <{}>",
           ecs::g_ecs_data->getDataComponents()->getName(comp.getComponentId()),
           ecs::g_ecs_data->getComponentTypes()->getName(comp.getTypeId()),
           eid.get_handle(),
           mgr->getEntityTemplateName(eid));
      //logerr("Attempt to serialize not-yet synced component <%s> of type <%s>, was entity %d<%s> re-created?",
      //       mgr->getDataComponents().getComponentNameById(comp.getComponentId()), mgr.getComponentTypes().getTypeNameById(comp.getTypeId()),
      //       (ecs::entity_id_t)eid, mgr.getEntityTemplateName(eid));
      return false;
    }
    write_component_index(comp.getComponentId(), bs);
    BitstreamSerializer serializer(*mgr, bs);
    ecs::serialize_entity_component_ref_typeless(comp, serializer, mgr);
    return true;
  }

  void Connection::writeLastRecvdPacketAcks(BitStream &bs)
  {
    auto ackData = reliabilitySys.genLastSeqAckData();
    bs.Write(ackData.rseq);
    bs.Write(ackData.ackBits);
  }
}
