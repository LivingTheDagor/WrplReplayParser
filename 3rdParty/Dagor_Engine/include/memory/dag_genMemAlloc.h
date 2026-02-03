//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once


#include "cstdint"

/// @addtogroup memory
/// @{


/// @file
/// Allocator classes for SmallTab.

struct IMemAlloc {
  void *alloc(int sz) {
    return malloc(sz);
  }

  void *allocAligned(size_t n, size_t al) {
    return _aligned_malloc(n, al);
  }

  void free(void *p) {
    return ::free(p);
  }

  bool resizeInplace(void *p, size_t sz) {
    return false;
  }

  void *realloc(void *p, size_t sz) {
    return ::realloc(p, sz);
  }
};

extern IMemAlloc G_ALLOC;


#define DECLARE_MEMALLOC(NAME, MEM)                                                      \
  struct NAME                                                                            \
  {                                                                                      \
    NAME()                                                                               \
    {}                                                                                   \
    explicit NAME(const char *)                                                                   \
    {}                                                                                   \
    static inline IMemAlloc *getMem()                                                    \
    {                                                                                    \
      return &(MEM);                                                                        \
    }                                                                                    \
    static inline void *alloc(int sz)                                                    \
    {                                                                                    \
      return (MEM).alloc(sz);                                                             \
    }                                                                                    \
    static inline void free(void *p)                                                     \
    {                                                                                    \
      return (MEM).free(p);                                                               \
    }                                                                                    \
    static inline void *allocate(size_t n, int /*flags*/ = 0)                            \
    {                                                                                    \
      return (MEM).alloc(n);                                                              \
    }                                                                                    \
    static inline void *allocate(size_t n, size_t al, size_t /*ofs*/, int /*flags*/ = 0) \
    {                                                                                    \
      return (MEM).allocAligned(n, al);                                                   \
    }                                                                                    \
    static inline void deallocate(void *p, size_t)                                       \
    {                                                                                    \
      (MEM).free(p);                                                                      \
    }                                                                                    \
    static inline bool resizeInplace(void *p, size_t sz)                                 \
    {                                                                                    \
      return (MEM).resizeInplace(p, sz);                                                  \
    }                                                                                    \
    static inline void *realloc(void *p, size_t sz)                                      \
    {                                                                                    \
      return (MEM).realloc(p, sz);                                                        \
    }                                                                                    \
    static inline void set_name(const char *)                                            \
    {}                                                                                   \
  }


// all the various allocs gaijin defines are supposed to allow for different allocators for specific uses, each controlling their own memory
// in practice im pretty sure it doesnt do that, and regardless I dont need that
DECLARE_MEMALLOC(MidmemAlloc, G_ALLOC);

DECLARE_MEMALLOC(InimemAlloc, G_ALLOC);

DECLARE_MEMALLOC(TmpmemAlloc, G_ALLOC);

DECLARE_MEMALLOC(StrmemAlloc, G_ALLOC);

DECLARE_MEMALLOC(UimemAlloc, G_ALLOC);

#undef DECLARE_MEMALLOC

/// @}
