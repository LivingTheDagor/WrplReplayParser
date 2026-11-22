#pragma once
#include "memory/dag_genMemAlloc.h"
#include <cstddef>
#include <cstdlib>
#include <memory_resource>
#include <new>
#if USE_MI_HEAP == 1 && defined(__linux__)
#include <sys/mman.h>
#endif
class StateRewinder;
// #define ASAN_ENABLED


/// ParserState allocator that is backed by a mimalloc heap
///
class StateAllocator : public std::pmr::memory_resource {
  struct StateDagAlloc : public IMemAlloc {
    StateDagAlloc() {
#if USE_MI_HEAP == 1
      heap = mi_heap_new();
#endif
    }
    ~StateDagAlloc() {
#if USE_MI_HEAP == 1
      mi_heap_destroy(heap);
#endif
    }
    void destroy() override {}
    bool isEmpty() override { return false; }
    size_t getSize(void *p) override { return 0; }
    void *alloc(size_t sz) override {
#if USE_MI_HEAP == 1
      return mi_heap_malloc(heap, sz);
#else
      return malloc(sz);
#endif
    }
    void *tryAlloc(size_t sz) override {
#if USE_MI_HEAP == 1
      return mi_heap_malloc(heap, sz);
#else
      return malloc(sz);
#endif
    }
    void *allocAligned(size_t sz, size_t alignment) override {
#if USE_MI_HEAP == 1
      return mi_heap_malloc_aligned(heap, sz, alignment);
#else
      return ::aligned_alloc(alignment, sz);
#endif
    }
    bool resizeInplace(void *p, size_t sz) override {
#if USE_MI_HEAP == 1
      return mi__expand(p, sz) != nullptr;
#else
      return false;
#endif
    }
    void *realloc(void *p, size_t sz) override {
#if USE_MI_HEAP == 1
      return mi_heap_realloc(heap, p, sz);
#else
      return ::realloc(p, sz);
#endif
    }
    void free(void *p) override {
#if USE_MI_HEAP == 1
      mi_free(p);
#else
      return ::free(p);
#endif
    }
    void freeAligned(void *p) override {
#if USE_MI_HEAP == 1
      mi_free(p);
#else
      return ::free(p);
#endif
    }
#if USE_MI_HEAP == 1
    mi_heap_t *heap = nullptr;
#endif
  };

  StateDagAlloc dagAlloc;

#if USE_MI_HEAP == 1 && defined(__linux__)
  // Blocks from this size up (the event lists of a long battle: 155 MB of ammo events on a large naval one)
  // come straight from the system and go straight back. mimalloc keeps a freed block resident for its purge
  // delay (1 s), so each time such a list grew, the block it left stayed next to the new one until the end of
  // the part. Only the memory_resource side knows the size of a block on its release, so only it does this.
  static constexpr std::size_t big_block_bytes = std::size_t(4) << 20;
  static bool is_big_block(std::size_t bytes, std::size_t alignment) {
    return bytes >= big_block_bytes && alignment <= 4096;
  }
#endif

  void *do_allocate(std::size_t bytes, std::size_t alignment) override {
#if USE_MI_HEAP == 1
#if defined(__linux__)
    if (is_big_block(bytes, alignment)) {
      void *p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (p == MAP_FAILED)
        throw std::bad_alloc();
      return p;
    }
#endif
    return mi_heap_malloc_aligned(dagAlloc.heap, bytes, alignment);
#else
    return ::malloc(bytes);
#endif
  }

  void do_deallocate(void *p, std::size_t bytes, std::size_t alignment) override {
#if USE_MI_HEAP == 1
#if defined(__linux__)
    if (is_big_block(bytes, alignment)) {
      munmap(p, bytes);
      return;
    }
#endif
    mi_free(p);
#else
    return ::free(p);
#endif
  }

  bool do_is_equal(std::pmr::memory_resource const &other) const noexcept override { return this == &other; }
  friend StateRewinder;

public:
  using DagAllocType = StateDagAlloc *;

  /// Objects of the state. Always from the state heap, never through the size-dependent mmap path of
  /// do_allocate: _delete is often called through a base pointer (battle messages, rewind events), so the
  /// size it could pass would be the size of the base, not of the object.
  template<class T, class... Args>
  T *_new(Args &&...args) {
#if USE_MI_HEAP == 1
    void *mem = mi_heap_malloc_aligned(dagAlloc.heap, sizeof(T), alignof(T));
#else
    static_assert(alignof(T) <= alignof(std::max_align_t), "over-aligned type without USE_MI_HEAP");
    void *mem = ::malloc(sizeof(T));
#endif
    if (!mem)
      throw std::bad_alloc();
    return new (mem) T(std::forward<Args>(args)...);
  }

  /// T may be a base of the object made by _new (it must then have a virtual destructor). The size is not
  /// used: the block is freed by address alone.
  template<class T>
  void _delete(const T *ptr) {
    if (!ptr)
      return;
    std::destroy_at(ptr);
#if USE_MI_HEAP == 1
    mi_free(const_cast<T *>(ptr));
#else
    ::free(const_cast<T *>(ptr));
#endif
  }
  StateAllocator() = default;
  ~StateAllocator() override = default;

  DagAllocType getMem() { return &dagAlloc; }
#if MI_USE_HEAP == 1
  mi_heap_t *get_heap_ptr() { return dagAlloc.heap; }
#endif
};

/// see _in_destruction_state, represents the allocator of the currently being destroyed state
inline thread_local StateAllocator *_in_destruction_allocator{};
