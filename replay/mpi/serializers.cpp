#include "mpi/serializers.h"
#include "ecs/entityId.h"
#include "network/eid.h"

namespace danet {

  int DefaultCoder(DANET_ENCODER_SIGNATURE) {
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      return false;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      return false;
    }
    return false;
  }

  int StringCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<std::string>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      bs->Read(*data);
      return true;
    }
    return false;
  }

  int TranslatedCoder(DANET_ENCODER_SIGNATURE) {
    return StringCoder(op, meta, ro, bs);
  }

  int boolCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<bool>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(*data));
      return true;
    }
    return false;
  }

  int UidCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<Uid>();
    char raw[82];
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      memset(raw, 0, sizeof(raw));
      bs->Write(data->pid);
      memcpy(raw, (void*)data->PLayerName.data(), data->PLayerName.size());
      bs->WriteArray(raw, sizeof(raw));
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(data->pid));
      REPL_VER(bs->ReadArray(raw, sizeof(raw)));
      auto strlen_val = strlen(raw);
      data->PLayerName = "";
      data->PLayerName.assign(raw, std::min(strlen_val, sizeof(raw)));
      return true;
    }
    return false;
  }

// TODO: change this when blk serialization is online
  int DataBlockCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<DataBlock>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      return false;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      uint32_t size;
      REPL_VER(bs->ReadCompressed(size));
      if (size > 0)
      {
        std::vector<char> raw;
        raw.resize(size);
        REPL_VER(bs->ReadArray(raw.data(), size));
        BaseReader rdr{raw.data(), (int)raw.size(), false};
        REPL_VER(data->loadFromStream(rdr, nullptr, nullptr));
      }
      return true;
    }
    return false;
  }

  int uint64Coder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<uint64_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(*data));
      return true;
    }
    return false;
  }

  int uint32Coder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<uint32_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(*data));
      return true;
    }
    return false;
  }

  int uint16Coder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<uint16_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(*data));
      return true;
    }
    return false;
  }

  int uint8Coder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<uint8_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(*data));
      return true;
    }
    return false;
  }

  int ULEB64Coder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<uint64_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->WriteCompressed(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->ReadCompressed(*data));
      return true;
    }
    return false;
  }

  int ULEB32Coder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<uint32_t>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->WriteCompressed(*data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->ReadCompressed(*data));
      return true;
    }
    return false;
  }

  int dummyForSupportPlanesCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<std::array<ecs::EntityId, 20>>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      for(auto &v : *data)
      {
        net::write_eid(*bs, v);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      for(auto &v : *data)
      {
        REPL_VER(net::read_eid(*bs, v));
      }
      return true;
    }
    return false;
  }

  int dummyForFootballStatCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<dummyForFootballStat>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(data->v1);
      bs->Write(data->v2);
      bs->Write(data->v3);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(data->v1));
      REPL_VER(bs->Read(data->v2));
      REPL_VER(bs->Read(data->v3));
      return true;
    }
    return false;
  }

  int RoundScoreCoder(DANET_ENCODER_SIGNATURE) {
    auto data = meta->getValue<RoundScore>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write(data->combined);
      bs->Write((uint8_t)data->scores.size());
      for(auto &v : data->scores)
      {
        bs->Write(v);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(bs->Read(data->combined));
      uint8_t len;
      REPL_VER(bs->Read(len));
      data->scores.resize(len);
      for(auto &v : data->scores)
      {
        REPL_VER(bs->Read(v));
      }
      return true;
    }
    return false;
  }

  int U8VectorCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<std::vector<uint8_t>>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write((uint8_t)data->size());
      for(auto &v : *data)
      {
        bs->Write(v);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      uint8_t len;
      REPL_VER(bs->Read(len));
      data->resize(len);
      for(auto &v : *data)
      {
        REPL_VER(bs->Read(v));
      }
      return true;
    }
    return false;
  }

  int U16VectorCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<std::vector<uint16_t>>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write((uint8_t)data->size());
      for(auto &v : *data)
      {
        bs->Write(v);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      uint8_t len;
      REPL_VER(bs->Read(len));
      data->resize(len);
      for(auto &v : *data)
      {
        REPL_VER(bs->Read(v));
      }
      return true;
    }
    return false;
  }

  int U32VectorCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<std::vector<uint32_t>>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->Write((uint8_t)data->size());
      for(auto &v : *data)
      {
        bs->Write(v);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      uint8_t len;
      REPL_VER(bs->Read(len));
      data->resize(len);
      for(auto &v : *data)
      {
        REPL_VER(bs->Read(v));
      }
      return true;
    }
    return false;
  }

  int EntityIdCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs)  {
    auto data = meta->getValue<ecs::EntityId>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      net::write_eid(*bs, *data);
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      REPL_VER(net::read_eid(*bs, *data));
      return true;
    }
    return false;
  }

  int PlayerStatCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<dummyForPlayerStat>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->reserveBits(BYTES_TO_BITS(sizeof(dummyForPlayerStat))); // easy optimization
      (bs->Write(data->v1));
      (bs->Write(data->v2));
      (bs->Write(data->v3));
      (bs->Write(data->v4));
      (bs->Write(data->v5));
      (bs->Write(data->v6));
      (bs->Write(data->v7));
      (bs->Write(data->v8));
      (bs->Write(data->v9));
      (bs->Write(data->v10));
      (bs->Write(data->v11));
      (bs->Write(data->v12));
      (bs->Write(data->v13));
      (bs->Write(data->v14));
      (bs->Write(data->v15));
      (bs->Write(data->v16));
      (bs->Write(data->v17));
      (bs->Write(data->v18));
      (bs->Write(data->v19));
      (bs->Write(data->v20));
      (bs->Write(data->v21));
      (bs->Write(data->v22));
      (bs->Write(data->v23));
      (bs->Write(data->v24));
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      // we dont know how the struct will be laid out in mem (ie: padding) so we must read individually
      REPL_VER(bs->Read(data->v1));
      REPL_VER(bs->Read(data->v2));
      REPL_VER(bs->Read(data->v3));
      REPL_VER(bs->Read(data->v4));
      REPL_VER(bs->Read(data->v5));
      REPL_VER(bs->Read(data->v6));
      REPL_VER(bs->Read(data->v7));
      REPL_VER(bs->Read(data->v8));
      REPL_VER(bs->Read(data->v9));
      REPL_VER(bs->Read(data->v10));
      REPL_VER(bs->Read(data->v11));
      REPL_VER(bs->Read(data->v12));
      REPL_VER(bs->Read(data->v13));
      REPL_VER(bs->Read(data->v14));
      REPL_VER(bs->Read(data->v15));
      REPL_VER(bs->Read(data->v16));
      REPL_VER(bs->Read(data->v17));
      REPL_VER(bs->Read(data->v18));
      REPL_VER(bs->Read(data->v19));
      REPL_VER(bs->Read(data->v20));
      REPL_VER(bs->Read(data->v21));
      REPL_VER(bs->Read(data->v22));
      REPL_VER(bs->Read(data->v23));
      REPL_VER(bs->Read(data->v24));
      return true;
    }
    return false;
  }

  int dummyForKillStreaksProgressCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<dummyForKillStreaksProgress>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    { // need 68, was
      bs->Write((uint8_t)data->vals.size());
      for(auto &v : data->vals) {
        bs->Write(v.v1);
        bs->Write(v.v2);
        bs->Write(v.v3);
      }
      return true;
    }
    if (op == DANET_REFLECTION_OP_DECODE)
    {
      uint8_t sz;
      REPL_VER(bs->Read(sz));
      data->vals.resize(sz);
      for(auto & v : data->vals) {
        REPL_VER(bs->Read(v.v1));
        REPL_VER(bs->Read(v.v2));
        REPL_VER(bs->Read(v.v3));
      }
      return true;
    }
    return false;
  }

  int dummyForCrewUnitsListCoder(int op, ReflectionVarMeta *meta, const ReflectableObject *ro, BitStream *bs) {
    auto data = meta->getValue<CrewUnitsList>();
    if(op == DANET_REFLECTION_OP_ENCODE)
    {
      bs->WriteCompressed((uint32_t)data->crew.size());
      for(auto &v : data->crew) {
        net::write_server_eid(v.e1, *bs);
        net::write_server_eid(v.e2, *bs);
        bs->Write(v.v1);
        bs->Write(v.v2);
      }
      return true;
    }
    if(op == DANET_REFLECTION_OP_DECODE)
    {
      uint32_t count;
      bool good = bs->ReadCompressed(count);
      data->crew.resize(count);
      for(auto &v : data->crew) {
        good &= net::read_server_eid(v.e1, *bs);
        good &= net::read_server_eid(v.e2, *bs);
        good &= bs->Read(v.v1);
        good &= bs->Read(v.v2);
      }
      return good;
    }
    return false;
  }
}