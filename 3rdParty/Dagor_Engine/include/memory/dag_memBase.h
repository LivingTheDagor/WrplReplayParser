
//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once
#include <cstdint>
#include <stddef.h>
class IMemAlloc
{
public:
  /// destroy this memory allocator
  virtual void destroy() = 0;

  /// returns true if allocator is empty (no more memory to allocate)
  virtual bool isEmpty() = 0;

  /// returns size of block
  /// @param p pointer returned from alloc/tryAlloc/realloc
  /// @note NULL is valid input; size of NULL = 0
  virtual size_t getSize(void *p) = 0;

  /**
    allocate memory.
    calls fatal on allocation error.
    @param sz size of requested block, in bytes
    @return pointer to allocated block or NULL on error
  */
  virtual void *alloc(size_t sz) = 0;

  /**
    fault-tolerant allocate memory.
    returns NULL on allocation error;
    @param sz size of requested block, in bytes
    @return pointer to allocated block or NULL on error
  */
  virtual void *tryAlloc(size_t sz) = 0;

  /**
    allocate aligned memory.
    calls fatal on allocation error.
    @param sz size of requested block, in bytes
    @param alignment of requested block, in bytes. If 0 - page size used. 16 - is default alloc alignment, don't bother to call with 16
    @return pointer to aligned block of memory
  */
  virtual void *allocAligned(size_t sz, size_t alignment) = 0;

  /**
    tries to resize block without moving it.
    @param sz new (expanded/shrinked) block size, in bytes
    @return returns true if succesfully expanded/shrinked or false when cannot expand
  */
  virtual bool resizeInplace(void *p, size_t sz) = 0;

  /**
    resize if possible, if not, alloc new, copy old to new, and free old block.
    Uses getsize().
    @return on error, returns NULL, but original block is preserved
  */
  virtual void *realloc(void *p, size_t sz) = 0;

  /// free previously allocated block.
  /// @param p pointer returned from alloc/tryAlloc/realloc
  /// @note NULL is valid input (it just does nothing)
  virtual void free(void *p) = 0;

  /// free block previously allocated with allocAligned()
  /// @param p pointer returned from allocAligned
  /// @note NULL is valid input (it just does nothing)
  virtual void freeAligned(void *p) = 0;

  //
  // inline versions of allocation functions that return size of allocated block
  //
  inline void *alloc(size_t sz, size_t *asize)
  {
    void *p = alloc(sz);
    *asize = sz;
    return p;
  }

  inline void *tryAlloc(size_t sz, size_t *asize)
  {
    void *p = tryAlloc(sz);
    *asize = sz;
    return p;
  }

  inline void *allocAligned(size_t sz, size_t alignment, size_t *asize)
  {
    void *p = allocAligned(sz, alignment);
    *asize = sz;
    return p;
  }

  inline bool resizeInplace(void *p, size_t sz, size_t *asize)
  {
    bool ret = resizeInplace(p, sz);
    *asize = getSize(p);
    if (!*asize)
      *asize = sz;
    return ret;
  }
};

#define DECL_MEM(name)                            \
extern IMemAlloc *name;                 \
inline IMemAlloc *name##_ptr() { return name; } \


DECL_MEM(stdmem);    //< standard c/c++ memory allocator
DECL_MEM(scriptmem); //< memory for scripts objects
DECL_MEM(strmem);    //< memory for strings objects
DECL_MEM(tmpmem);    //< memory for small temporary objects
DECL_MEM(inimem);    //< memory for objects allocated on init and released on shutdown
DECL_MEM(midmem);    //< memory for objects that remain in memory for significant time
DECL_MEM(uimem);     //< memory for UI-related objects
DECL_MEM(globmem);
DECL_MEM(defaultmem); //< memory used by default (in operator new)