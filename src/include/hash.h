

#ifndef MYEXTENSION_HASH_H
#define MYEXTENSION_HASH_H

#include "wyhash.h"
#include <stdint.h>

template <int HashBits>
struct FNV1Params;

template <>
struct FNV1Params<32>
{
  typedef uint32_t HashVal;
  static constexpr uint32_t offset_basis = 2166136261U;
  static constexpr uint32_t prime = 16777619;
};

template <>
struct FNV1Params<64>
{
  typedef uint64_t HashVal;
  static constexpr uint64_t offset_basis = 14695981039346656037LU;
  static constexpr uint64_t prime = 1099511628211;
};

template <int HashBits>
using HashVal = typename FNV1Params<HashBits>::HashVal;

template <int HashBits>
constexpr HashVal<HashBits> str_hash_fnv1(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  HashVal<HashBits> c = 0;
  while ((c = *s++) != 0)
    result = (FNV1Params<HashBits>::prime) * (result ^ c);
  return result;
}

template <int HashBits>
constexpr HashVal<HashBits> mem_hash_fnv1(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  for (size_t i = 0; i < len; ++i)
  {
    HashVal<HashBits> c = b[i];
    result = (result * FNV1Params<HashBits>::prime) ^ c;
  }
  return result;
}

// by default use 32-bit FNV1
constexpr HashVal<32> str_hash_fnv1(const char *s) { return str_hash_fnv1<32>(s); }

constexpr HashVal<32> mem_hash_fnv1(const char *b, size_t len) { return mem_hash_fnv1<32>(b, len); }


#endif //MYEXTENSION_HASH_H
