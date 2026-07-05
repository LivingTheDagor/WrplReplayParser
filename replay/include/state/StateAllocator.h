#pragma once
#include "mimalloc.h"
#include <memory_resource>
#include "memory/dag_genMemAlloc.h"

/// ParserState allocator that is backed by a mimalloc heap
///
class StateAllocator : public std::pmr::memory_resource {


  struct StateDagAlloc : public IMemAlloc {
    StateDagAlloc() {
      heap = mi_heap_new();
    }
    ~StateDagAlloc() {
      mi_heap_delete(heap);
    }
    void destroy() override {}
    bool isEmpty() override {return false;}
    size_t getSize(void *p) override {return 0;}
    void *alloc(size_t sz) override {
      return mi_heap_malloc(heap, sz);
    }
    void *tryAlloc(size_t sz) override {
      return mi_heap_malloc(heap, sz);
    }
    void *allocAligned(size_t sz, size_t alignment) override {
      return mi_heap_malloc_aligned(heap, sz, alignment);
    }
    bool resizeInplace(void *p, size_t sz) override {
      return mi_expand(p, sz) != nullptr;
    }
    void *realloc(void *p, size_t sz) override {
      return mi_heap_realloc(heap, p, sz);
    }
    void free(void *p) override {
      mi_free(p);
    }
    void freeAligned(void *p) override {
      mi_free(p);
    }
    mi_heap_t * heap = nullptr;
  };

  StateDagAlloc dagAlloc;

  void * do_allocate( std::size_t bytes, std::size_t alignment ) override
  {
    return mi_heap_malloc_aligned(dagAlloc.heap, bytes, alignment);
  }

  void do_deallocate( void * p, std::size_t bytes, std::size_t alignment ) override
  {
    mi_free(p);
  }

  bool do_is_equal( std::pmr::memory_resource const& other ) const noexcept override
  {
    return this == &other;
  }
public:
  StateAllocator() = default;
  ~StateAllocator() = default;

  IMemAlloc * getMem() {
    return &dagAlloc;
  }
};