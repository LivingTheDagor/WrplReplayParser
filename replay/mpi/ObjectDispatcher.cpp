#include "math/dag_TMatrix.h"
#include "mpi/ObjectDispatcher.h"
#include "ecs/EntityManager.h"
#include "zstd.h"
#include "state/ParserState.h"
#include "mpi/GeneralObject.h"
#include "Unit.h"
#include "mpi/PositionSync.h"
#include "state/ParserState.h"

CREATE_HANDLE(handle_object_dispatcher, "ObjectDispatcher")

namespace mpi {
  void zstd_decompress(BitStream &in, BitStream &out) {
    ZoneScoped;
    uint32_t comp_size;
    uint32_t decomp_size;
    in.ReadCompressed(comp_size);
    in.ReadCompressed(decomp_size);
    out.Reset();
    out.reserveBits(BYTES_TO_BITS(decomp_size));
    out.SetWriteOffset(BYTES_TO_BITS(decomp_size));
    std::vector<char> inData{};
    inData.resize(comp_size);
    in.ReadArray(inData.data(), comp_size);
    ZSTD_decompress(out.GetData(), decomp_size, inData.data(), inData.size());
  }

  Message *GeneralObject::dispatchMpiMessage(MessageID mid) {
    ZoneScoped;
    // LOG("incoming mid: 0x%x\n", mid);
    switch (mid) {
      case Replication:
      case Reflection1:
      case Reflection2:
      case ReflectionNoDecompress: {
        return state->_new<Message>(this, mid);
        //return new Message(this, mid);
        break;
      }
      case Kill: {
        return state->_new<KillMessage>(this);
        ///return new KillMessage(this);
        DISPATCHER_LOGD1("KILL");
        break;
      }
      case Awards: {
        return state->_new<AwardMessage>(this);
        ///return new AwardMessage(this);
        DISPATCHER_LOGD1("Awards");
        break;
      }
      case SevereDamage: {
        return state->_new<SevereDamageMessage>(this);
        DISPATCHER_LOGD1("SevereDamage");
        break;
      }
      case CriticalDamage: {
        return state->_new<CriticalDamageMessage>(this);
        //return new CriticalDamageMessage(this);
        DISPATCHER_LOGD1("CriticalDamage");
        break;
      }
      case Tank1:
      case Tank2: {
        DISPATCHER_LOGD1("Tank");
        return state->_new<TankMessage>(this, mid);
        //return new TankMessage(this, mid);
      }
      case Rocket1:
      case Rocket2: {
        return state->_new<BSMessage>(this, mid);
        //return new BSMessage(this, mid);
      }
    }
    // LOG("no mid found\n");
    return nullptr;
  }

  void GeneralObject::applyMpiMessage(const Message *m) {
    ZoneScoped;
    auto mid = m->id;
    auto bs = (BitStream *) &m->payload;
    // LOG("Deserialzing for Reflection type: %0x\n", mid);
    switch (mid) {
      case Kill: {
        const KillMessage *kill_m = dynamic_cast<const KillMessage *>(m);
        if (kill_m->offended_unit) {
          kill_m->offended_unit->killed_at_ms = this->state->curr_time_ms;
        }
        [[fallthrough]];
      }
      case SevereDamage:
      case CriticalDamage:
      case Awards: {
        m->delete_message = false;
        const IBattleMessage *battle_m = dynamic_cast<const IBattleMessage *>(m);
        this->state->BattleMessages.push_back(battle_m);
        break;
      }
      case ReflectionNoDecompress: {
        danet::deserializeReflectables(*bs, obj_dispatcher, this->state);
        break;
      }
      case Reflection1:
      case Reflection2: {
        uint8_t tmp = 0;
        bs->Read(tmp);
        bool isCompressed = tmp == 1;
        BitStream *outBs;
        BitStream t{};
        if (isCompressed) {
          zstd_decompress(*bs, t);
          outBs = &t;
        } else {
          outBs = bs;
        }
        danet::deserializeReflectables(*outBs, obj_dispatcher, this->state);
        break;
      }
      case Replication: {
        uint8_t do_weird_check;
        bs->Read(do_weird_check);
        int16_t sorta_confirms_is_compressed;
        bs->Read(sorta_confirms_is_compressed);
        if (sorta_confirms_is_compressed > -1) {
          danet::ReplicatedObject::onRecvReplicationEvent(*bs, this->state);
        } else {
          BitStream t{};
          zstd_decompress(*bs, t);
          danet::ReplicatedObject::onRecvReplicationEvent(t, this->state);
        }
        break;
      }
      case Tank1:
      case Tank2: {
        auto tankM = (TankMessage *) m;
        auto ret = GMSync(*state, tankM->data);
        if (!ret) {
          LOGE("Failed to read TankMessage payload");
        }
        return;
      }
      case Rocket1:
      case Rocket2: {
        auto bsM = (BSMessage *) m;
        bool ret = WeaponSync(*state, bsM->data);
        if (!ret) {
          LOGE("Failed to read We payload");
        }
        return;
      }
    }
  }

  ecs::EntityId eid_from_ext_uid(uint32_t ext_uid) { return ecs::EntityId((ext_uid & 0xFF) << 0x16 | ext_uid >> 0x8); }

  enum ExtMpiTypes {
    FMW = 0,
    GM = 1,
    INF = 2,
    T_3 = 3,
    T_4 = 4,
    T_5 = 5,
    T_6 = 6,
    T_7 = 7,
    T_8 = 8,
    T_9 = 9,
    T_10 = 10,
    T_11 = 11,
    FM_FX = 12,
    UFX = 13,
    T_14 = 14,
    T_15 = 15,
    FM_DVM = 16, // fm and gm have it in the same place,
    GM_DVM = 17,
    CUD = 18,
    T_19 = 19,
    T_20 = 20,
    T_21 = 21,
    T_22 = 22,
    T_23 = 23,
    T_24 = 24,
    WEAP = 25,
  };

  IObject *MpiQueueObject::UnitRef_Dispatch(ObjectID oid, ObjectExtUID extUid, ParserState *state, bool do_queue) {
    ZoneScoped;
    if (!extUid) {
      EXCEPTION("dispatch: extended mpi uid is not set for object of type {}", oid >> 0xb);
    }

    auto eid = ecs::EntityId(extUid);
    unit::UnitRef* ref=nullptr;
    {
      ZoneScopedN("unit::UnitRef_getNullable");
      ref = state->g_entity_mgr.getNullable<unit::UnitRef>(eid, ECS_HASH("unit__ref"));
    }
    if (!ref) {
      if (do_queue) {
        // when the ref is null, gaijin assumes it has net yet been created, so push it to the queue
        state->mpi_queue.set_oid_ext_uid(oid, extUid);
        return &state->mpi_queue;
      } else {
        LOGE("Warning, Entity {:#x} doesn't exist even after queue dispatch, find your error.", eid.get_handle());
        return nullptr;
      }
    }
    if (!ref->unit)
      return nullptr;
    auto unit = ref->unit;
    auto obj_type = oid >> 0xb;
    if (obj_type == FMW && unit->unitType == UnitType::AircraftType) {
      return unit->base_data;
    }
    // should also later include fortifications?
    if (obj_type == GM && unit->unitType == UnitType::TankType) {
      return unit->base_data;
    }
    if (obj_type == WEAP) {
      return &unit->weapons_mask;
    }
    return nullptr;
  }

  Message *MpiQueueObject::dispatchMpiMessage(MessageID mid) {
    return nullptr; // don't have any unit message parsing setup for now
  }

  void MpiQueueObject::applyMpiMessage(const Message *m) {
    ZoneScopedN("MpiQueueObject::applyMpiMessage");
    if (m) {
      auto eid = eid_from_ext_uid(this->mpiObjectExtUID);
      auto &queue_data = this->dispatched_objects[eid].emplace_back();
      auto &bs = queue_data.bs;
      bs.reserveBits(m->payload.GetNumberOfBitsUsed() + 8 + 32 + 64); // 64 is average header
      auto before_header_write = bs.GetWriteOffset();
      bs.IgnoreBytes(4);
      auto before_write = bs.GetWriteOffset();
      write_object_ext_uid(bs, this);
      queue_data.oid = this->mpiObjectUID;
      queue_data.extUid = this->mpiObjectExtUID;
      bs.Write(m->id);
      bs.Write(m->payload);
      bs.AlignWriteToByteBoundary();
      uint32_t write_offs = ((bs.GetWriteOffset() - before_write) >> 3) | MPI;
      auto after_all_write = bs.GetWriteOffset();
      bs.SetWriteOffset(before_header_write);
      bs.Write(write_offs);
      bs.SetWriteOffset(after_all_write);
    }
  }

  bool MpiQueueObject::deserialize(BitStream &other_bs, int data_size, ParserState *state) {
    ZoneScopedN("MpiQueueObject::deserialize");
    auto eid = eid_from_ext_uid(this->mpiObjectExtUID);
    auto &queue_data = this->dispatched_objects[eid].emplace_back();
    auto &bs = queue_data.bs;
    bs.reserveBits(BYTES_TO_BITS(data_size) + 8 + 32);

    uint32_t write_offs = data_size | REFL;
    bs.Write(write_offs);
    queue_data.oid = this->mpiObjectUID;
    queue_data.extUid = this->mpiObjectExtUID;

    bs.WriteBits(other_bs.GetData() + BITS_TO_BYTES(other_bs.GetReadOffset()), BYTES_TO_BITS(data_size));
    other_bs.IgnoreBytes(data_size);
    return true;
  }

  IObject *ObjectDispatcher(ObjectID oid, ObjectExtUID extUid, ParserState *state) {
    ZoneScoped;
    uint16_t count = oid & 0x7ff;
    uint8_t obj = (uint8_t) (oid >> 0xb);
    switch (obj) {
      case FMW:
      case GM:
      case INF:
      case FM_FX:
      case UFX:
      case FM_DVM:
      case GM_DVM:
      case CUD:
      case WEAP:
        G_ASSERT(extUid != INVALID_OBJECT_EXT_UID);
        return MpiQueueObject::UnitRef_Dispatch(oid, extUid, state, true);
      case 3: break;
      case 5: {
        // LOGE("Getting Zone with id {}", count);
        if (count < state->Zones.size()) {
          return state->Zones[count];
        } else {
          DISPATCHER_LOGE("Warning, Zone with id {} doesn't exist in Zone array", count);
        }
        break;
      }
      case 0x8: {
        // something to do with mission objectives, lookup "extendedEnding"
        return nullptr;
      }
      case 0xb: {
        switch (count) {
          case 0x2: {
            return &state->main_dispatch;
          }
          case 0x3: {
            return nullptr; // want to silence the "unable to dispatch with this"
          }
          case 0x4: {
            return &state->gen_state;
          }
          case 0x8: {
            return &state->glob_elo;
          }
        }
        break;
      }
      case 0xe: {
        // LOG("Getting Player id %i\n", count);
        if (count < state->players.size())
          return &state->players[count];
        break;
      }
      case 0xf: {
        if (count < 3) {
          // max team count is 3
          return &state->teams[count];
        }
        EXCEPTION("Invalid team index");
      }
      case 0x16: {
        if (count < state->missionAreas2.size()) {
          auto var = state->missionAreas2[count];
          if (!var)
            LOGE("Invalid MissionArea index");
          return var;
        } else {
          // LOGE("Invalid MissionArea index"); // this can actually be expected behavior apparently.
        }
        return nullptr;
      }
    }
    // LOG("unable to dispatch to oid: {:#x}; type: {}; index: {}", oid, obj, count);
    return nullptr;
  }

  bool TankMessage::readPayload(ParserState *state) { return this->payload.Read(this->data); }

  void TankMessage::writePayload() { this->payload.Write(this->data); }
} // namespace mpi

ECS_REGISTER_CTM_TYPE(MPlayer, nullptr);
ECS_AUTO_REGISTER_COMPONENT_BASE(MPlayer, "m_player", nullptr)
