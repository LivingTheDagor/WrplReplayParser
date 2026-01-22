

#ifndef MYEXTENSION_BITSTREAMDESERIALIZER_H
#define MYEXTENSION_BITSTREAMDESERIALIZER_H
#include "BitStream.h"
#include "ComponentTypes.h"
#include "DataComponents.h"

struct InternedStrings
{
  std::unordered_map<std::string, uint32_t> index;
  std::vector<std::string> strings; // if strings[index] - "", it is not synced. strings[0] = ''
  InternedStrings();
};
struct BitstreamDeserializer final : public ecs::DeserializerCb
{
  const BitStream &bs;
  InternedStrings *objectKeys = nullptr;
  ecs::EntityManager *mgr;
  BitstreamDeserializer(const BitStream &bs_, ecs::EntityManager *mgr, InternedStrings *keys = nullptr) : bs(bs_), objectKeys(keys) {}
  bool read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const override;
  bool skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo);
};

struct BitstreamSerializer final : public ecs::SerializerCb
{
  BitStream &bs;
  ecs::EntityManager *mgr;
  InternedStrings *objectKeys = nullptr;
  BitstreamSerializer(ecs::EntityManager &mgr, BitStream &bs_, InternedStrings *keys = nullptr) :
      bs(bs_), mgr(&mgr), objectKeys(keys)
  {}
  void write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type) override;
};
namespace ecs
{
  int read_string(const ecs::DeserializerCb &cb, char *buf, uint32_t buf_size);
  void write_string(ecs::SerializerCb &cb, const char *pStr, uint32_t max_string_len);
} // ecs
#endif //MYEXTENSION_BITSTREAMDESERIALIZER_H
