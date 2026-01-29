
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <mpi/reflection.h>
#include <idFieldSerializer.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Quat.h>
#include <math/dag_adjpow2.h>
#include <algorithm>

#define debug(...) logmessage(_MAKE4C('DNET'), __VA_ARGS__)

#define DEBUG_BORDERS_REFLECTION  0
#define DEBUG_BORDERS_REPLICATION 0

namespace danet
{

  List<ReflectableObject, true> changed_reflectables;
  List<ReflectableObject, true> all_reflectables;
  ReplicatedObjectCreator registered_repl_obj_creators[DANET_REPLICATION_MAX_CLASSES];
  int num_registered_obj_creators = 0;
  uint32_t option_flags = 0;

  struct SortCreator
  {
    inline bool operator()(const ReplicatedObjectCreator &l, const ReplicatedObjectCreator &r) const
    {
      return strcmp(l.name, r.name) < 0;
    }
  };

  void normalizeReplication()
  {
    std::sort(registered_repl_obj_creators, registered_repl_obj_creators + num_registered_obj_creators, SortCreator());
    for (int i = 0; i < num_registered_obj_creators; ++i)
      *registered_repl_obj_creators[i].class_id_ptr = i;
  }

  void on_reflection_var_created(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, uint8_t persistent_id,
                                 const char *var_name, uint16_t var_flags, uint16_t var_bits)
  {
    robj->checkWatermark();
#if DAGOR_DBGLEVEL > 0
    if (!(var_flags & RVF_EXCLUDED))
  {
    const danet::ReflectionVarMeta *sameNameMeta = robj->searchVarByName(var_name);
    G_ASSERTF(!sameNameMeta, "duplicate reflection var {}", var_name);
    const danet::ReflectionVarMeta *sameIdMeta = robj->getVarByPersistentId(persistent_id);
    G_ASSERTF(!sameIdMeta, "duplicate reflection var id {}", persistent_id);
  }
#endif
    if (robj->varList.tail)
    {
      robj->varList.tail->next = meta;
      robj->varList.tail = meta;
    }
    else
      robj->varList.tail = robj->varList.head = meta;
    meta->next = NULL;
    meta->persistentId = persistent_id;
    meta->name = var_name;
    meta->flags = var_flags;
    meta->numBits = var_bits;
  }

  void on_reflection_var_changed(danet::ReflectionVarMeta *meta, danet::ReflectableObject *robj, void *new_val)
  {
    robj->checkWatermark();
    if ((meta->flags & RVF_EXCLUDED) || (robj->isRelfectionFlagSet(danet::ReflectableObject::EXCLUDED)))
    {
      if (meta->flags & RVF_CALL_HANDLER_FORCE)
        robj->onReflectionVarChanged(meta, new_val);
      return;
    }
    if (!(meta->flags & RVF_CHANGED))
    {
      meta->flags |= RVF_CHANGED;
      DANET_DLIST_PUSH(danet::changed_reflectables, robj, prevChanged, nextChanged);
    }
    if (meta->flags & (RVF_CALL_HANDLER | RVF_CALL_HANDLER_FORCE))
      robj->onReflectionVarChanged(meta, new_val);
  }

  void ReflectableObject::markVarWithFlag(ReflectionVarMeta *var, uint16_t f, bool set)
  {
    checkWatermark();

    G_ASSERT(f);         // what the hell?
#if DAGOR_DBGLEVEL > 0 // make sure this var is belong to us
    bool fnd = false;
  for (const ReflectionVarMeta *v = varList.head; v && !fnd; v = v->next)
    fnd = v == var;
  G_ASSERT(fnd);
#endif
    if (set)
      var->flags |= f;
    else
      var->flags &= ~f;
  }

  int ReflectableObject::serialize(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
                                   bool do_reset_changed_flag)
  {
    checkWatermark();

    if (reflectionFlags & EXCLUDED)
      return 0;

    G_ASSERT(flags_to_have || fth_all);

    ReflectionVarMeta *varsToWrite[DANET_REFLECTION_MAX_VARS_PER_OBJECT];
    G_STATIC_ASSERT(sizeof(varsToWrite) <= 4096); // to much stack?
    G_STATIC_ASSERT(DANET_REFLECTION_MAX_VARS_PER_OBJECT <= IdFieldSerializer255::MAX_FIELDS_NUM);

    BitSize_t data_size_pos = ~0u;
    // first pass - fill in indexes only
    BitSize_t numVarsPos = 0;
    uint32_t numSerializedVars = 0; // in current reflectable
    IdFieldSerializer255 idFieldSerializer;
    for (ReflectionVarMeta *v = varList.head; v; v = v->next)
    {
      if (v->flags & RVF_EXCLUDED)
        continue;

      const bool allIgnoreFlagsExists = flags_to_ignore && (v->flags & flags_to_ignore) == flags_to_ignore;
      const bool anyIgnoreFlagsExists = (v->flags & flags_to_ignore) != 0;
      if (fti_all ? allIgnoreFlagsExists : anyIgnoreFlagsExists)
        continue;

      const bool allFlagsExists = (v->flags & flags_to_have) == flags_to_have;
      const bool anyFlagsExists = (v->flags & flags_to_have) != 0;

      if (!flags_to_have || (fth_all ? allFlagsExists : anyFlagsExists))
      {
        if (numSerializedVars == 0) // write header on first var write
        {
          bs.Write((uint16_t)0); // reserve space for data size (for skip in case if deserialization fails)
          data_size_pos = bs.GetWriteOffset();
          mpi::write_object_ext_uid(bs, getUID(), mpiObjectExtUID);
          bs.AlignWriteToByteBoundary();
          numVarsPos = bs.GetWriteOffset();
          bs.Write((uint16_t)0);                                 // reserve space for offset to sizes
          bs.Write(static_cast<IdFieldSerializer255::Index>(0)); // reserve space for vars count
        }

        idFieldSerializer.setFieldId(v->persistentId);
        varsToWrite[numSerializedVars++] = v;
      }
    }

    if (numSerializedVars)
    {
      G_ASSERT(numSerializedVars <= DANET_REFLECTION_MAX_VARS_PER_OBJECT);
      G_ASSERT(data_size_pos != ~0u);

      // second pass - write var data
      for (int i = 0; i < numSerializedVars; ++i)
      {
        ReflectionVarMeta *v = varsToWrite[i];
        G_ASSERT(v->coder);
#if DEBUG_BORDERS_REFLECTION
        bs.Write((uint8_t)117);
#endif
        BitSize_t pos_before_write = bs.GetWriteOffset();
        (*v->coder)(DANET_REFLECTION_OP_ENCODE, v, this, &bs);
        BitSize_t pos_after_write = bs.GetWriteOffset();
        G_ASSERTF(pos_after_write > pos_before_write, "{} in object '{}' var coder {} for var '{}' ({}) did not writed any data",
                  __FUNCTION__, getClassName(), fmt::ptr((void *)v->coder), v->getVarName(), fmt::ptr(v));
        idFieldSerializer.setFieldSize(pos_after_write - pos_before_write);

#if DAGOR_DBGLEVEL > 0
        if ((option_flags & DUMP_REFLECTION) || (v->flags & RVF_DEBUG) || (reflectionFlags & DEBUG_REFLECTION))
        debug("%s serialize var '%s'(0x%p == 0x%x) %d bytes in obj '%s'(0x%p)", __FUNCTION__, v->getVarName(), v, v->getValue<int>(),
          BITS_TO_BYTES(v->numBits), getClassName(), this);
#endif

#if DEBUG_BORDERS_REFLECTION
        bs.Write((uint8_t)118);
#endif
        if (do_reset_changed_flag)
          v->flags &= ~RVF_CHANGED;
      }

      // third pass - write var ids and var data sizes
      idFieldSerializer.writeFieldsIndex(bs, numVarsPos);
      idFieldSerializer.writeFieldsSize(bs, numVarsPos);
      BitSize_t curPos = bs.GetWriteOffset();

      // write total size of writed data
      uint32_t total_data_size = BITS_TO_BYTES(curPos - data_size_pos);
      G_ASSERT(total_data_size <= 65535); // max - 64K per reflectable
      bs.WriteAt((uint16_t)total_data_size, data_size_pos - (BitSize_t)BYTES_TO_BITS(sizeof(uint16_t)));
#if DAGOR_DBGLEVEL > 0
      if (option_flags & DUMP_REFLECTION)
      debug("%s serialized object 0x%p '%s' %u vars %u(+2) bytes", __FUNCTION__, this, getClassName(), numSerializedVars,
        total_data_size);
#endif
    }

    if (do_reset_changed_flag)
      checkAndRemoveFromChangedList();

    return numSerializedVars;
  }

  int serializeChangedReflectables(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
                                   bool do_reset_changed_flag)
  {
    if (!changed_reflectables.head)
      return 0;

    BitSize_t writeNumReflPos = bs.GetWriteOffset();
    bs.Write((uint16_t)0); // make room for num serialized reflectables

    int numSerializedReflectables = 0;
    for (ReflectableObject *r = changed_reflectables.head; r;)
    {
      ReflectableObject *nextChanged = r->nextChanged;
      r->checkWatermark();

      if ((r->isRelfectionFlagSet(ReflectableObject::EXCLUDED)) == 0 && r->isNeedSerialize())
      {
        BitSize_t pos2_before_write = bs.GetWriteOffset();
        (void)pos2_before_write;
        if (r->serialize(bs, flags_to_have, fth_all, flags_to_ignore, fti_all, do_reset_changed_flag))
        {
          G_ASSERT(bs.GetWriteOffset() - pos2_before_write); // nothing was written, but returned true?
          bs.AlignWriteToByteBoundary();
          uint32_t data_written = BITS_TO_BYTES(bs.GetWriteOffset() - pos2_before_write);
          (void)data_written;
          G_ASSERTF(data_written <= 65535, "{} {} type='{}' writed more than 65535 byte data ({})", __FUNCTION__, fmt::ptr(r), r->getClassName(),
                    data_written);
          ++numSerializedReflectables;
        }
      }
      else if (do_reset_changed_flag)
        r->resetChangeFlag();

      r = nextChanged;
    }

    // write reflectable count
    G_ASSERT(numSerializedReflectables <= 65535);
    bs.WriteAt((uint16_t)numSerializedReflectables, writeNumReflPos);

    return numSerializedReflectables;
  }

  int serializeAllReflectables(BitStream &bs, uint16_t flags_to_have, bool fth_all, uint16_t flags_to_ignore, bool fti_all,
                               bool do_reset_changed_flag)
  {
    if (!all_reflectables.head)
      return 0;

    BitSize_t writeNumReflPos = bs.GetWriteOffset();
    bs.Write((uint16_t)0); // make room for num serialized reflectables

    int numSerializedReflectables = 0;
    for (ReflectableObject *r = all_reflectables.head; r;)
    {
      ReflectableObject *next = r->next;

      if ((r->isRelfectionFlagSet(ReflectableObject::EXCLUDED)) == 0 && r->isNeedSerialize())
      {
        BitSize_t pos2_before_write = bs.GetWriteOffset();
        (void)pos2_before_write;
        if (r->serialize(bs, flags_to_have, fth_all, flags_to_ignore, fti_all, do_reset_changed_flag))
        {
          G_ASSERT(bs.GetWriteOffset() - pos2_before_write); // nothing was written, but returned true?
          bs.AlignWriteToByteBoundary();
          uint32_t data_written = BITS_TO_BYTES(bs.GetWriteOffset() - pos2_before_write);
          (void)data_written;
          G_ASSERTF(data_written <= 65535, "{} {} type='{}' writed more than 65535 byte data ({})", __FUNCTION__, fmt::ptr(r), r->getClassName(),
                    data_written);
          ++numSerializedReflectables;
        }
      }
      else if (do_reset_changed_flag)
        r->resetChangeFlag();

      r = next;
    }

    // write reflectable count
    G_ASSERT(numSerializedReflectables <= 65535);
    bs.WriteAt((uint16_t)numSerializedReflectables, writeNumReflPos);

    return numSerializedReflectables;
  }

  int forceFullSyncForAllReflectables()
  {
    int ret = 0;
    for (ReflectableObject *r = all_reflectables.head; r; r = r->next, ++ret)
      r->forceFullSync();
    return ret;
  }

  bool ReflectableObject::deserialize(BitStream &bs, int /* data_size */)
  {
    checkWatermark();

    BitSize_t end_pos = 0;
    IdFieldSerializer255 idFieldSerializer;
    // read var sizes
    uint32_t numVars = idFieldSerializer.readFieldsSizeAndCount(bs, end_pos);
    G_ASSERT(numVars <= DANET_REFLECTION_MAX_VARS_PER_OBJECT);

    ReflectionVarMeta *varsToRead[DANET_REFLECTION_MAX_VARS_PER_OBJECT];
    G_STATIC_ASSERT(sizeof(varsToRead) <= 4096); // to much stack?
    G_STATIC_ASSERT(DANET_REFLECTION_MAX_VARS_PER_OBJECT <= IdFieldSerializer255::MAX_FIELDS_NUM);

    // read var indexes
    idFieldSerializer.readFieldsIndex(bs);
    ReflectionVarMeta *v = varList.head;
    //LOG("Deserializing Vars for %p: ", this);
    for (uint16_t j = 0; (uint32_t)j < numVars; ++j)
    {
      v = getVarByPersistentId(idFieldSerializer.getFieldId(j));
      if (v)
      {
        G_ASSERT((v->flags & RVF_NEED_DESERIALIZE) == 0);
        v->flags |= RVF_NEED_DESERIALIZE; // mark vars that we going to deserialize
        //LOG("%s; ", v->name);
      }
      varsToRead[j] = v;
    }
    //LOG("\n");

    // allow objects choose what flags to deserialize
    onBeforeVarsDeserialization();

    // read var data
    bool ret = true;
    for (uint16_t j = 0; j < numVars; ++j)
    {
      v = varsToRead[j];
      //int old_value = v ? *v->getValue<int>() : 0;
      BitSize_t ppp = bs.GetReadOffset();
      if (!v)
      {
        idFieldSerializer.skipReadingField(j, bs); // skip

        continue;
      }
      G_ASSERT(v->coder);
      int out = (*v->coder)(DANET_REFLECTION_OP_DECODE, v, this, &bs);
      if (!out)
      {
        LOGE("(REFLECTION) can't decode value for var '{}' in obj {} (type = '{}')", v->getVarName(), fmt::ptr(this), getClassName());
        idFieldSerializer.skipReadingField(j, bs); // skip
        continue;
      }
      if (bs.GetReadOffset() - ppp != idFieldSerializer.getFieldSize(j))
      {
        if (out != 2)
          LOGE("(REFLECTION) Invalid parsed size for var '{}', needed {} but parsed {}", v->getVarName(), idFieldSerializer.getFieldSize(j), bs.GetReadOffset() - ppp);
        // e.g. both Ship and HVM have the same MPI_OID_GROUND_MODEL and the same reflection variables persistent ids
        // but WarShip fields really differ from those of HeavyVehicleModel
        bs.SetReadOffset(ppp);
        idFieldSerializer.skipReadingField(j, bs); // skip
        //ret = false;
        continue;
      }

      G_ASSERT(!(v->flags & RVF_DESERIALIZED));
      if (v->flags & RVF_NEED_DESERIALIZE)
        v->flags |= RVF_DESERIALIZED;
    }

    bs.SetReadOffset(end_pos);

    // tell to object that we done
    onAfterVarsDeserialization();

    // reset service flags
    for (v = varList.head; v; v = v->next)
      v->flags &= ~(RVF_DESERIALIZED | RVF_NEED_DESERIALIZE);

    return ret;
  }

  int deserializeReflectables(BitStream &bs, mpi::object_dispatcher resolver, ParserState *state)
  {
    G_ASSERT(resolver);

    uint16_t numReflectables = 65535;
    if (!bs.Read(numReflectables))
      return -1;

    for (int i = 0; i < numReflectables; ++i)
    {
      uint16_t data_written = 0xffff;
      if (!bs.Read(data_written))
        return -2;

      G_ASSERTF(BITS_TO_BYTES(bs.GetNumberOfUnreadBits()) >= data_written,
                "{} unexpected stream end - unreaded {} bytes, data_written={} bytes", __FUNCTION__, BITS_TO_BYTES(bs.GetNumberOfUnreadBits()),
                data_written);
      if (BITS_TO_BYTES(bs.GetNumberOfUnreadBits()) < data_written)
        return -3;

      BitSize_t start_pos = bs.GetReadOffset();
      mpi::ObjectID oid = mpi::INVALID_OBJECT_ID;
      mpi::ObjectExtUID extUid = mpi::INVALID_OBJECT_EXT_UID;
      mpi::read_object_ext_uid(bs, oid, extUid);
      bs.AlignReadToByteBoundary();
      BitSize_t startPosAfterId = bs.GetReadOffset();
      ReflectableObject *refl = (oid == mpi::INVALID_OBJECT_ID && extUid == mpi::INVALID_OBJECT_EXT_UID)
                                ? NULL
                                : static_cast<ReflectableObject *>(resolver(oid, extUid, state));
      if (refl && refl->debugWatermark != DANET_WATERMARK)
        LOG("can't resolve (or bad reflection object) {} by oid {:#x}\n", fmt::ptr(refl), oid);
      bool read_fail = false;
      if (!refl || refl->debugWatermark != DANET_WATERMARK ||
          !refl->deserialize(bs, int(data_written) - BITS_TO_BYTES(int(startPosAfterId - start_pos))))
        read_fail = true;
      else
      {
        bs.AlignReadToByteBoundary();
        uint32_t real_readed = bs.GetReadOffset() - start_pos;
        real_readed = BITS_TO_BYTES(real_readed);
        read_fail = real_readed != data_written;
        G_ASSERTF(real_readed == data_written,
                  "{} integrity check for reflection packet failed, "
                  "written ({}) != readed ({}) bytes, obj = {} ({:#x}, {})",
                  __FUNCTION__, data_written, real_readed, fmt::ptr(refl), (int)oid, refl ? refl->getClassName() : "");
      }
      if (read_fail)
      {
        bs.SetReadOffset(start_pos);
        bs.IgnoreBytes(data_written);
      }
    }

    return numReflectables;
  }

  void ReplicatedObject::genReplicationEvent(BitStream &out_bs) const
  {
    G_ASSERT(getClassId() <= 255);

#if DEBUG_BORDERS_REPLICATION
    out_bs.Write((uint8_t)113);
#endif

    out_bs.Write((uint8_t)getClassId());
    serializeReplicaCreationData(out_bs);

#if DEBUG_BORDERS_REPLICATION
    out_bs.Write((uint8_t)114);
#endif
  }

/* static */
  ReplicatedObject *ReplicatedObject::onRecvReplicationEvent(BitStream &bs, ParserState *state)
  {

    uint8_t classId = 255;
    if (!bs.Read(classId))
    {
      LOG("can't read classId");
      return NULL;
    }
    if (classId >= num_registered_obj_creators)
    {
      LOG("invalid classId {}, num_registered_obj_creators = {}\n", classId, num_registered_obj_creators);
      return NULL;
    }
    ReplicatedObject *ret = (*registered_repl_obj_creators[classId].create)(bs, state);

    return ret;
  }

  void ReplicatedObject::genReplicationEventForAll(BitStream &bs)
  {
    BitSize_t pos = bs.GetWriteOffset();
    bs.Write((uint16_t)0);
    int numSerialized = 0;
    for (ReflectableObject *obj = all_reflectables.head; obj; obj = obj->next)
      if (obj->isRelfectionFlagSet(ReflectableObject::REPLICATED))
      {
        static_cast<ReplicatedObject *>(obj)->genReplicationEvent(bs);
        ++numSerialized;
      }
    G_ASSERT(numSerialized <= 65535);
    bs.WriteAt((uint16_t)numSerialized, pos);
  }

  bool ReplicatedObject::onRecvReplicationEventForAll(BitStream &bs, ParserState *state)
  {
    uint16_t n;
    if (!bs.Read(n))
      return false;
    for (int i = 0; i < n; ++i)
      if (!ReplicatedObject::onRecvReplicationEvent(bs, state))
        return false;
    return true;
  }
}; // namespace danet