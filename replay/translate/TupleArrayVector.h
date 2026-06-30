#pragma once
#include "array"
#include "vector"

#include <cassert>
#include <cstdint>
#include <utility>

template<typename T, std::size_t count, T default_value>
class TupleArrayVector {
  std::array<T *, count> ptrArray; // an array of pointers to each sub array
  // these elements are only used purely for resize ops, which for my current use case won't occur often
  uint32_t Count{}; // sub array length in element count
  uint32_t Capacity{}; // sub array capacity in element count
  T *data = nullptr;

  void calculatePtrArray(T *ptr, uint32_t capacity);

  /// handles resize operation
  void allocateAndMove(uint32_t newCapacity);

  /// handles dynamic resizing
  void grow();


public:
  /// resizes vector so that each array can hold this many elements
  /// @param newSize The new size, in elements, that each sub array should hold
  void resize(size_t newSize);
  /// shrinks the vector to fit the current number of elements
  void shrink();

  void reserve(uint32_t newCapacity);
  /// ensures you have enough space for one more element in each sub array.
  /// if not, does the standard allocation
  void ensureOne();

  /// returns reference to some element inside some array
  T &at(size_t arrayIndex, size_t elementIndex) const;

  /// raw access to list of pointers to each sub array
  const std::array<T *, count> &raw() const;

  explicit TupleArrayVector(size_t start_elements);

  uint32_t size() const { return Count; }

  ~TupleArrayVector();

  TupleArrayVector(const TupleArrayVector &) = delete;
  TupleArrayVector &operator=(const TupleArrayVector &) = delete;

  TupleArrayVector(TupleArrayVector &&other) noexcept;
  TupleArrayVector &operator=(TupleArrayVector &&other) noexcept;
};


template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::calculatePtrArray(T *ptr, uint32_t capacity) {
  for (size_t i = 0; i < count; i++) {
    ptrArray[i] = ptr + (i * capacity);
  }
}

template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::allocateAndMove(uint32_t newCapacity) {
  if (newCapacity == Capacity)
    return;
  T *newData = new T[newCapacity * count];
  std::array<T *, count> oldPtrArray = ptrArray;
  calculatePtrArray(newData, newCapacity);


  if (data && newCapacity > 0) {

    // move data when we have it
    for (size_t i = 0; i < count; i++) {
      auto oldPtr = oldPtrArray[i];
      auto newPtr = ptrArray[i];
      for (size_t j = 0; j < Capacity; j++) {
        newPtr[j] = std::move(oldPtr[j]);
      }
    }
  }

  // fill new data with default value
  if (newCapacity > Capacity) {
    for (size_t i = 0; i < count; i++) {
      auto newPtr = ptrArray[i];
      for (size_t j = Capacity; j < newCapacity; j++) {
        newPtr[j] = default_value;
      }
    }
  }

  delete[] data;
  data = newData;
  Capacity = newCapacity;
}
template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::grow() {
  if (Capacity == 0) {
    reserve(1);
  } else {
    reserve(Capacity * 2);
  }
}

template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::resize(size_t newSize) {
  // fuck downsizing
  if (newSize <= Count)
    return;
  if (newSize > Capacity) {
    reserve(newSize);
  }
  Count = newSize;
}

template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::shrink() {
  if (Capacity > Count) {
    allocateAndMove(Count);
  }
}

template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::reserve(uint32_t newCapacity) {
  if (newCapacity <= Capacity)
    return;
  allocateAndMove(newCapacity);
}

template<typename T, std::size_t count, T default_value>
void TupleArrayVector<T, count, default_value>::ensureOne() {
  if (Count >= Capacity) {
    grow();
  }
  Count++;
}

template<typename T, std::size_t count, T default_value>
T &TupleArrayVector<T, count, default_value>::at(size_t arrayIndex, size_t elementIndex) const {
  assert(arrayIndex < count);
  assert(elementIndex < Count);
  return ptrArray[arrayIndex][elementIndex];
}
template<typename T, std::size_t count, T default_value>
const std::array<T *, count> &TupleArrayVector<T, count, default_value>::raw() const {
  return ptrArray;
}
template<typename T, std::size_t count, T default_value>
TupleArrayVector<T, count, default_value>::TupleArrayVector(size_t start_elements) {
  reserve((uint32_t) start_elements);
  Count = 0;
}
template<typename T, std::size_t count, T default_value>
TupleArrayVector<T, count, default_value>::~TupleArrayVector() {
  delete[] data;
  data = nullptr;
  Count = 0;
  Capacity = 0;
}

template<typename T, std::size_t count, T default_value>
TupleArrayVector<T, count, default_value>::TupleArrayVector(TupleArrayVector &&other) noexcept :
  ptrArray(other.ptrArray), Count(other.Count), Capacity(other.Capacity), data(other.data) {
  other.ptrArray.fill(nullptr);
  other.Count = 0;
  other.Capacity = 0;
  other.data = nullptr;
}

template<typename T, std::size_t count, T default_value>
TupleArrayVector<T, count, default_value> &
TupleArrayVector<T, count, default_value>::operator=(TupleArrayVector &&other) noexcept {
  if (this != &other) {
    delete[] data;

    ptrArray = other.ptrArray;
    Count = other.Count;
    Capacity = other.Capacity;
    data = other.data;

    other.ptrArray.fill(nullptr);
    other.Count = 0;
    other.Capacity = 0;
    other.data = nullptr;
  }
  return *this;
}
