#pragma once
#include "mimalloc.h"
#include <memory_resource>
#include "memory/dag_genMemAlloc.h"

class StateRewinder;
//#define ASAN_ENABLED
#ifndef ASAN_ENABLED
#define USE_MI_HEAP
#endif


/// ParserState allocator that is backed by a mimalloc heap
///
class StateAllocator : public std::pmr::memory_resource {
  struct StateDagAlloc : public IMemAlloc {
    StateDagAlloc() {
#ifdef USE_MI_HEAP
      heap = mi_heap_new();
#endif
    }
    ~StateDagAlloc() {
#ifdef USE_MI_HEAP
      mi_heap_delete(heap);
#endif
    }
    void destroy() override {}
    bool isEmpty() override {return false;}
    size_t getSize(void *p) override {return 0;}
    void *alloc(size_t sz) override {
#ifdef USE_MI_HEAP
      return mi_heap_malloc(heap, sz);
#else
      return malloc(sz);
#endif
    }
    void *tryAlloc(size_t sz) override {
#ifdef USE_MI_HEAP
      return mi_heap_malloc(heap, sz);
#else
      return malloc(sz);
#endif
    }
    void *allocAligned(size_t sz, size_t alignment) override {
#ifdef USE_MI_HEAP
      return mi_heap_malloc_aligned(heap, sz, alignment);
#else
      return ::aligned_alloc(alignment, sz);
#endif
    }
    bool resizeInplace(void *p, size_t sz) override {
#ifdef USE_MI_HEAP
      return mi__expand(p, sz) != nullptr;
#else
      return false;
#endif
    }
    void *realloc(void *p, size_t sz) override {
#ifdef USE_MI_HEAP
      return mi_heap_realloc(heap, p, sz);
#else
      return ::realloc(p, sz);
#endif
    }
    void free(void *p) override {
#ifdef USE_MI_HEAP
      mi_free(p);
#else
      return ::free(p);
#endif
    }
    void freeAligned(void *p) override {
#ifdef USE_MI_HEAP
      mi_free(p);
#else
      return ::free(p);
#endif
    }
    mi_heap_t * heap = nullptr;
  };

  StateDagAlloc dagAlloc;

  void * do_allocate( std::size_t bytes, std::size_t alignment ) override
  {
#ifdef USE_MI_HEAP
    return mi_heap_malloc_aligned(dagAlloc.heap, bytes, alignment);
#else
    return ::aligned_alloc(alignment, bytes);
#endif
  }

  void do_deallocate( void * p, std::size_t bytes, std::size_t alignment ) override
  {
#ifdef USE_MI_HEAP
    mi_free(p);
#else
    return ::free(p);
#endif
  }

  bool do_is_equal( std::pmr::memory_resource const& other ) const noexcept override
  {
    return this == &other;
  }
  friend StateRewinder;
public:
  using DagAllocType = StateDagAlloc*;

  template <class T, class... Args>
  T * _new(Args&&... args) {
  void * mem = do_allocate(sizeof(T), alignof(T));
  return new (mem) T(std::forward<Args>(args)...);
}

  template <class T>
  void _delete(const T * ptr) {
  if (ptr) {
    if (!ptr) return;
    std::destroy_at(ptr);
    do_deallocate(static_cast<void*>(const_cast<T*>(ptr)), sizeof(T), alignof(T));
  }
}
  StateAllocator() = default;
  ~StateAllocator() override = default;

  DagAllocType getMem() {
    return &dagAlloc;
  }
};