

#ifndef MYEXTENSION_ECSHASH_H
#define MYEXTENSION_ECSHASH_H



#include "hash.h"
#include "string_view"


typedef uint32_t hash_str_t;

template <int HashBits>
constexpr HashVal<HashBits> ecs_str_hash(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return str_hash_fnv1a<HashBits>(s, result);
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_mem_hash(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return mem_hash_fnv1a<HashBits>(b, len, result);
}

constexpr hash_str_t ecs_mem_hash(const char *b, size_t len) { return ecs_mem_hash<32>(b, len); }
constexpr hash_str_t ecs_str_hash(const char *b) { return ecs_str_hash<32>(b); }

struct HashedConstString
{
  const char *str;
  hash_str_t hash;
};


#define ECS_HASH_SLOW(a) (HashedConstString{a, ecs_str_hash(a)})
#define ECS_HASH(a) ECS_HASH_SLOW(a)



#endif //MYEXTENSION_ECSHASH_H
