#include "ecs/BitStreamDeserializer.h"
#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include "network/eid.h"
#include "consts.h"
#include "ecs/baseIo.h"

InternedStrings::InternedStrings() {
  index.emplace("", 0);
  strings.emplace_back("");
}

namespace ecs {
  //extern int MAX_STRING_LENGTH;

  void write_string(ecs::SerializerCb &cb, const char *pStr, uint32_t max_string_len) {
    for (const char *str = pStr; *str && str - pStr < max_string_len; str++)
      cb.write(str, sizeof(str[0]) * CHAR_BIT, 0);
    const char c = 0;
    cb.write(&c, sizeof(c) * CHAR_BIT, 0);
  }

  int read_string(const ecs::DeserializerCb &cb, char *buf, uint32_t buf_size) {
    buf_size--;
    char *str = buf;
    do {
      if (!cb.read(str, 8, 0))
        return -1;
      if (str == buf + buf_size)
        *str = 0;
    } while (*(str++));
    return (int)(str - buf);
  }

  static int read_string(const BitStream &cb, std::string &to) {
    char buf[1024];
    buf[0] = 0;
    for (char *str = buf;;) {
      if (!cb.Read(str, 1))
        return -1;
      if (!*str)
        break;
      if (str == buf + countof(buf) - 2) {
        str[1] = 0;
        to += buf;
        str = buf;
      } else
        str++;
    };
    to += buf;
    return (int)to.length();
  }

  inline bool read_string_no(const BitStream &cb, uint32_t &str, const uint32_t short_bits) {
    // assume Little endian
    str = 0;
    return cb.ReadBits((uint8_t *) &str, short_bits);
  }

  static std::string oneString;

  static const char *read_istring(const BitStream &cb, InternedStrings *istrs, uint32_t short_bits) {
    bool rawString = false;
    if (!cb.Read(rawString))
      return nullptr;
    if (rawString) {
      oneString.clear();
      if (read_string(cb, oneString) < 0)
        return nullptr;
      return oneString.c_str();
    } else if (!istrs)
      return nullptr;
    uint32_t str;
    if (!read_string_no(cb, str, short_bits))
      return nullptr;
    InternedStrings &all = *istrs;
    if (str < all.strings.size() && (str == 0 || all.strings[str].length())) {
      return all.strings[str].c_str();
    }
    if (str >= all.strings.size())
      all.strings.resize(str + 1);
    G_ASSERT(all.strings.size() <= uint32_t(1 << short_bits));
    std::string &it = all.strings[str];
    if (read_string(cb, it) < 0)
      return nullptr;
    return it.c_str();
  }

  inline void write_string_no(BitStream &cb, uint32_t str, const uint32_t short_bits)
  {
    // assume Little endian
    G_ASSERT(str < uint32_t(1 << short_bits));
    cb.WriteBits((const uint8_t *)&str, short_bits);
  }
  static void write_raw_string(BitStream &cb, const std::string &pStr)
  {
    cb.Write(true);
    cb.Write(pStr.c_str(), (uint32_t)pStr.length() + 1);
  }

  static void write_string(BitStream &cb, const std::string &pStr, InternedStrings &all, uint32_t short_bits)
  {
    auto it = all.index.find(pStr);
    if (it == all.index.end())
    {
      if (all.strings.size() >= uint32_t(1 << short_bits))
      {
        write_raw_string(cb, pStr);
        return;
      }
      cb.Write(false);
      write_string_no(cb, (uint32_t)all.strings.size(), short_bits);
      all.strings.emplace_back(pStr);
      cb.Write(all.strings.back().c_str(), (uint32_t)all.strings.back().length() + 1);
      all.index.emplace(all.strings.back(), all.strings.size() - 1);
    }
    else
    {
      cb.Write(false);
      write_string_no(cb, it->second, short_bits);
    }
}
} // ecs
static constexpr int OBJECT_KEY_BITS = 10;

bool BitstreamDeserializer::read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const {
  if (user_type == 0)
    return bs.ReadBits((uint8_t *) to, (uint32_t)sz_in_bits);
  else if (user_type == ecs::ComponentTypeInfo<ecs::EntityId>::type) {
    //DAECS_EXT_ASSERT(sz_in_bits == sizeof(ecs::EntityId) * CHAR_BIT);
    return net::read_eid(bs, *(ecs::EntityId *) to);
  } else if (user_type == ecs::ComponentTypeInfo<bool>::type) // bool optimization. Bool is actually one bit
  {
    G_ASSERT(sz_in_bits == CHAR_BIT);
    return bs.Read(*(bool *) to);
  } else if (user_type == ecs::ComponentTypeInfo<ecs::Object>::type) // intern strings for Objects
  {
    ecs::Object &obj = *((ecs::Object *) to);
    obj.clear();
    uint32_t cnt;
    if (!ecs::read_compressed(*this, cnt))
      return false;
    obj.reserve(cnt);
    for (uint32_t i = 0; i < cnt; ++i) {
      const char *str = ecs::read_istring(bs, objectKeys, OBJECT_KEY_BITS);
      if (!str)
        return false;
      auto &item = obj.insert(str); // insert before deserialization since 'str' might be reused in next calls
      if (ecs::MaybeComponent maybeComp = ecs::deserialize_child_component(*this, mgr))
        item = std::move(*maybeComp);
      else
        return false;
    }
    return true;
  } else if (user_type == ecs::ComponentTypeInfo<ecs::string>::type) {
    std::vector<char> tmp;
    tmp.resize(ecs::MAX_STRING_LENGTH);
    if (ecs::read_string(*this, tmp.data(), ecs::MAX_STRING_LENGTH) < 0)
      return false;
    *((ecs::string *) to) = tmp.data();
    return true;
  } else
    return bs.ReadBits((uint8_t *) to, (uint32_t)sz_in_bits);

}

bool BitstreamDeserializer::skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo) {
  if (compInfo.componentIndex == ecs::INVALID_COMPONENT_TYPE_INDEX)
    return false;
  auto componentTypes = ecs::g_ecs_data->getComponentTypes();
  const auto componentTypeInfo = componentTypes->getComponentData(compInfo.componentIndex);
  const bool isPod = ecs::is_pod(componentTypeInfo->flags);
  ecs::ComponentSerializer *typeIO = nullptr;
  if (compInfo.flags & ecs::DataComponent::HAS_SERIALIZER)
    typeIO = ecs::g_ecs_data->getDataComponents()->getTypeIo(cidx);
  if (!typeIO && has_io(componentTypeInfo->flags))
    typeIO = componentTypeInfo->serializer;
  void *tempData = alloca(componentTypeInfo->size);
  ecs::ComponentTypeManager *ctm = NULL;
  if (need_constructor(componentTypeInfo->flags))
  {
    ctm = componentTypes->getCTM(compInfo.componentIndex);
    G_ASSERTF(ctm, "type manager for type {:#x} ({}) missing", compInfo.hash, compInfo.componentIndex);
  }
  if (ctm)
    ctm->create(tempData, *mgr, ecs::INVALID_ENTITY_ID, compInfo.componentIndex);
  else if (!isPod)
    memset(tempData, 0, componentTypeInfo->size);
  bool ret =
      typeIO ? typeIO->deserialize(*this, tempData, componentTypeInfo->size, compInfo.componentHash, mgr)
             : read(tempData, componentTypeInfo->size * CHAR_BIT, compInfo.componentHash);
  if (ctm)
    ctm->destroy(tempData);
  return ret;
}

void BitstreamSerializer::write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type)
{
  if (user_type == 0)
    bs.WriteBits((const uint8_t *)from, (uint32_t)sz_in_bits);
  else if (user_type == ecs::ComponentTypeInfo<ecs::EntityId>::type)
  {
    G_ASSERT(sz_in_bits == sizeof(ecs::entity_id_t) * CHAR_BIT);
    net::write_server_eid(*(const ecs::entity_id_t *)from, bs);
  }
  else if (user_type == ecs::ComponentTypeInfo<bool>::type) // bool optimization
  {
    G_ASSERT(sz_in_bits == CHAR_BIT);
    bs.Write(*(bool *)from); // optimization
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::Object>::type) // intern strings for Objects
  {
    const ecs::Object &obj = *((const ecs::Object *)from);
    ecs::write_compressed(*this, obj.size());
    if (objectKeys)
    {
      for (auto &it : obj)
      {
        ecs::write_string(bs, ecs::get_key_string(it.first), *objectKeys, OBJECT_KEY_BITS);
        ecs::serialize_child_component(it.second, *this, mgr);
      }
    }
    else
    {
      for (auto &it : obj)
      {
        ecs::write_raw_string(bs, ecs::get_key_string(it.first));
        ecs::serialize_child_component(it.second, *this, mgr);
      }
    }
  }
  else if (user_type == ecs::ComponentTypeInfo<ecs::string>::type)
    ecs::write_string(*this, ((const ecs::string *)from)->c_str(), ecs::MAX_STRING_LENGTH);
  else
    bs.WriteBits((const uint8_t *)from, (uint32_t)sz_in_bits);
}