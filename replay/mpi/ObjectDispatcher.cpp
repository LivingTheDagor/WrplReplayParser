#include "mpi/ObjectDispatcher.h"
#include "ecs/EntityManager.h"
#include "zstd.h"
#include "state/ParserState.h"

namespace mpi {
  void zstd_decompress(BitStream &in, BitStream &out) {
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

    Message *MainDispatch::dispatchMpiMessage(MessageID mid) {
      //LOG("incoming mid: 0x%x\n", mid);
      switch(mid) {
        case Replication:
        case Reflection1:
        case Reflection2:
        case ReflectionNoDecompress: {
          return new Message(this, mid);
          break;
        }
        case Kill: {
          //LOG("KILL");
          break;
        }
        case Awards: {
          //LOG("Awards");
          break;

        }
        case SevereDamage: {
          //LOG("SevereDamage");
          break;

        }
        case CriticalDamage: {
          LOG("CriticalDamage");
          break;

        }

      }
      //LOG("no mid found\n");
      return nullptr;
    }
    void MainDispatch::applyMpiMessage(const Message *m) {
      auto mid = m->id;
      auto bs = (BitStream *)&m->payload;
      //LOG("Deserialzing for Reflection type: %0x\n", mid);
      switch(mid) {
        case ReflectionNoDecompress: {
          danet::deserializeReflectables(*bs, obj_dispatcher, this->state);
          break;
        }
        case Reflection1:
        case Reflection2: {
          uint8_t tmp;
          bs->Read(tmp);
          bool isCompressed = tmp == 1;
          BitStream * outBs;
          BitStream t{};
          if(isCompressed) {
            zstd_decompress(*bs, t);
            outBs = &t;
          }
          else
          {
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
          if(sorta_confirms_is_compressed > -1) {
            danet::ReplicatedObject::onRecvReplicationEvent(*bs, this->state);
          }
          else {
            BitStream t{};
            zstd_decompress(*bs, t);
            danet::ReplicatedObject::onRecvReplicationEvent(t, this->state);

          }
        }
      }
    }


  IObject * UnitRef_Dispatch(ObjectID oid, ObjectExtUID extUid, ecs::EntityManager *mgr) {
    if(!extUid)
    {
      EXCEPTION("dispatch: extended mpi uid is not set for object of type {}", oid>>0xb);
    }
    //auto eid = ecs::EntityId(extUid << 0x16 | extUid >> 8);
    //auto ref = mgr->getNullable<unit::UnitRef>(eid, ECS_HASH("unit__ref"));
    // TODO
    return nullptr;
  }
  IObject *ObjectDispatcher(ObjectID oid, ObjectExtUID extUid, ParserState *state) {
    uint16_t count = oid & 0x7ff;
    uint8_t obj = (uint8_t)(oid >> 0xb);
    switch(obj) {
      case 1:
      case 2:
        return UnitRef_Dispatch(oid, extUid, &state->g_entity_mgr);
      case 3:
        break;
      case 5: {
        LOG("Getting Zone with id {}", count);
        if (count < state->Zones.size()) {
          return state->Zones[count];
        }
        else {
          LOGE("Warning, Zone with id {} doesn't exist in Zone array", count);
        }
        break;
      }
      case 0xb: {
        switch(count) {
          case 0x2: {
            return &state->main_dispatch;
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
        //LOG("Getting Player id %i\n", count);
        if(count < state->players.size())
          return &state->players[count];
        break;
      }
      case 0xf: {
        if(count < 3) { // max team count is 3
          return &state->teams[count];
        }
      }
    }
    return nullptr;
  }
}

ECS_REGISTER_CTM_TYPE(MPlayer, nullptr);
ECS_AUTO_REGISTER_COMPONENT_BASE(MPlayer, "m_player", nullptr)