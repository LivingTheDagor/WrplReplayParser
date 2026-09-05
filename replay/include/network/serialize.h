//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ecs/typesAndLimits.h"


#include <ecs/baseIo.h>

class framemem_allocator;
namespace eastl {
  template<typename T, typename A>
  class vector;
  template<typename A, typename T, typename C>
  class bitvector;
} // namespace eastl

class BitStream;
namespace ecs {
  struct DataComponent;
  class EntityManager;
} // namespace ecs

namespace net {

  struct InternedStringsBase;
  struct InternedStringsRepl;
  class IConnection;

  struct BitstreamDeserializer final : public ecs::DeserializerCb {
    const BitStream &bs;
    ecs::EntityManager &mgr;
    InternedStringsRepl *objectKeys = nullptr;
    BitstreamDeserializer(ecs::EntityManager &mgr, const BitStream &bs_, InternedStringsRepl *keys = nullptr) :
      bs(bs_), mgr(mgr), objectKeys(keys) {}
    bool read(void *to, size_t sz_in_bits, ecs::component_type_t user_type) const override;
    bool skip(ecs::component_index_t cidx, const ecs::DataComponent &compInfo);
  };
  struct BitstreamSerializer final : public ecs::SerializerCb {
    BitStream &bs;
    ecs::EntityManager &mgr;
    InternedStringsRepl *objectKeys = nullptr;
    using ObjectKeysBitVector = eastl::bitvector<eastl::allocator, uint64_t>;
    ObjectKeysBitVector *outObjectKeysUsed = nullptr;
    BitstreamSerializer(ecs::EntityManager &mgr, BitStream &bs_) : bs(bs_), mgr(mgr) {}
    BitstreamSerializer(ecs::EntityManager &mgr, BitStream &bs_, InternedStringsRepl *object_keys,
                        ObjectKeysBitVector *out_keys_used) :
      bs(bs_), mgr(mgr), objectKeys(object_keys), outObjectKeysUsed(out_keys_used) {}
    void write(const void *from, size_t sz_in_bits, ecs::component_type_t user_type) override;
  };

} // namespace net
