#pragma once
#include "dag/dag_vector.h"
#include "memory/dag_genMemAlloc.h"
#include <array>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

// block allocator is an allocate once, free all container.
// allows you to incrementally create objs while preserving old references in a cache friendly way

// T is type, count is number of elements per block

template<class T, uint32_t COUNT = 16, class Alloc = TmpmemAlloc>
class BlockAllocator {
  static_assert(COUNT > 0, "BlockAllocator COUNT must be > 0");

  struct Slot {
    alignas(T) uint8_t data[sizeof(T)];
  };

  using Block = std::array<Slot, COUNT>;
  using BlockPtr = std::unique_ptr<Block>;
  dag::Vector<BlockPtr, Alloc> blocks;
  // records next element to return
  uint32_t next_block = 0, next_index = 0;
  uint32_t size_ = 0;

  T *ptrAt(uint32_t block_index, uint32_t index_in_block) {
    return reinterpret_cast<T *>(blocks[block_index]->at(index_in_block).data);
  }

  const T *ptrAt(uint32_t block_index, uint32_t index_in_block) const {
    return reinterpret_cast<const T *>(blocks[block_index]->at(index_in_block).data);
  }

public:
  BlockAllocator(Alloc alloc = Alloc{}) : blocks(alloc) {}
  ~BlockAllocator() { clear(); }

  T &operator[](uint32_t index) {
    G_ASSERT(index < size_);
    uint32_t block_index = index / COUNT;
    uint32_t index_in_block = index % COUNT;
    return *ptrAt(block_index, index_in_block);
  }

  const T &operator[](uint32_t index) const {
    G_ASSERT(index < size_);
    uint32_t block_index = index / COUNT;
    uint32_t index_in_block = index % COUNT;
    return *ptrAt(block_index, index_in_block);
  }

  uint32_t size() const { return size_; }

  BlockAllocator(const BlockAllocator &) = delete;
  BlockAllocator &operator=(const BlockAllocator &) = delete;
  BlockAllocator(BlockAllocator &&) noexcept = default;
  BlockAllocator &operator=(BlockAllocator &&) noexcept = default;

  void clear() {
    uint32_t remaining = size_;
    for (uint32_t b = 0; b < blocks.size() && remaining > 0; b++) {
      const uint32_t in_block = remaining > COUNT ? COUNT : remaining;
      for (uint32_t i = 0; i < in_block; i++)
        ptrAt(b, i)->~T();
      remaining -= in_block;
    }
    blocks.clear();
    next_block = 0;
    next_index = 0;
    size_ = 0;
  }

  template<class... Args>
  T &getNext(Args &&...args) {
    if (next_block >= blocks.size()) {
      blocks.push_back(std::make_unique<Block>());
    }

    T *result = ::new (blocks[next_block]->at(next_index).data) T(std::forward<Args>(args)...);
    ++size_;
    ++next_index;
    if (next_index == COUNT) {
      next_index = 0;
      ++next_block;
    }
    return *result;
  }
};
