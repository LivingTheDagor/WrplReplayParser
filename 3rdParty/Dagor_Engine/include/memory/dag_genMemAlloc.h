//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once


#include "cstdint"
#include "dag_memBase.h"
#include "mimalloc.h"

/// @addtogroup memory
/// @{


/// @file
/// Allocator classes for SmallTab.

struct GLOBAL_ALLOC: public IMemAlloc {
  void destroy() override {}
  bool isEmpty() override {return true;}
  void *alloc(size_t sz) override {
    return mi_malloc(sz);
  }

  void *tryAlloc(size_t sz) override {
    return mi_malloc(sz);
  }
  size_t getSize(void *p) override {
    return 0;
  }
  void freeAligned(void *p) override {
    return mi_free(p);
  }

  void *allocAligned(size_t n, size_t al) override {
    return mi_aligned_alloc(n, al);
  }

  void free(void *p) override {
    return mi_free(p);
  }

  bool resizeInplace(void *p, size_t sz) override {
    return mi_expand(p, sz) != nullptr;
  }

  void *realloc(void *p, size_t sz) override {
    return mi_realloc(p, sz);
  }
};

extern GLOBAL_ALLOC G_ALLOC;


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
