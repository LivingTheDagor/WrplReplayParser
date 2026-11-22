#pragma once
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

/// Append-only history in fixed segments: 2, 2, 4, 8, ... elements up to about 64 KB, then 64 KB each.
/// Elements never move once pushed, so a long history neither copies itself on growth nor keeps a
/// doubled buffer of which half is unused. Indexing is a shift and a mask; push is a pointer bump.
/// Not contiguous: use forEachSegment() for bulk reads.
template<typename TS>
class SegmentedHistory {
  static constexpr uint32_t S0 = 2;
  static constexpr uint32_t S =
    (65536 / sizeof(TS)) >= 32 ? std::bit_floor<uint32_t>(uint32_t(65536 / sizeof(TS))) : 32;
  static constexpr uint32_t SHIFT_S = std::countr_zero(S);
  static constexpr uint32_t SHIFT_S0 = std::countr_zero(S0);
  /// segments 0 .. K0-1 cover the indices [0, S)
  static constexpr uint32_t K0 = SHIFT_S - SHIFT_S0 + 1;
  static constexpr uint32_t N_INLINE = 4;

  TS *next = nullptr;    // free slot in the current segment
  TS *seg_end = nullptr; // end of the current segment
  TS **segs = inl;       // inl, or a heap table once there are more than N_INLINE segments
  uint32_t count = 0;
  uint32_t entered = 0; // segments in use (the current one is entered - 1)
  uint32_t nseg = 0;    // segments allocated (the last ones may be empty after pop_back)
  uint32_t segcap = N_INLINE;
  TS *inl[N_INLINE]{};

  static uint32_t segElems(uint32_t k) { return k == 0 ? S0 : (k < K0 ? (S0 << (k - 1)) : S); }

  static void locate(uint32_t i, uint32_t &k, uint32_t &o) {
    if (i >= S) {
      k = K0 + ((i - S) >> SHIFT_S);
      o = (i - S) & (S - 1);
    } else if (i < S0) {
      k = 0;
      o = i;
    } else {
      const uint32_t b = std::bit_width(i) - 1;
      k = b - SHIFT_S0 + 1;
      o = i - (1u << b);
    }
  }

  void addSeg() {
    if (nseg == segcap) {
      const uint32_t ncap = segcap * 2;
      TS **t = static_cast<TS **>(malloc(sizeof(TS *) * ncap));
      if (!t)
        throw std::bad_alloc();
      memcpy(t, segs, sizeof(TS *) * nseg);
      if (segs != inl)
        free(segs);
      segs = t;
      segcap = ncap;
    }
    TS *p = static_cast<TS *>(malloc(sizeof(TS) * segElems(nseg)));
    if (!p)
      throw std::bad_alloc();
    segs[nseg++] = p;
  }

  /// called when the current segment is full: go to the next one, allocate it if needed
  void advance() {
    if (entered == nseg)
      addSeg();
    next = segs[entered];
    seg_end = next + segElems(entered);
    entered++;
  }

  void release() noexcept {
    if constexpr (!std::is_trivially_destructible_v<TS>)
      forEachSegmentMut([](TS *p, uint32_t n) {
        for (uint32_t i = 0; i < n; i++)
          p[i].~TS();
      });
    for (uint32_t k = 0; k < nseg; k++)
      free(segs[k]);
    if (segs != inl)
      free(segs);
  }

  void stealFrom(SegmentedHistory &o) noexcept {
    next = o.next;
    seg_end = o.seg_end;
    count = o.count;
    entered = o.entered;
    nseg = o.nseg;
    segcap = o.segcap;
    if (o.segs == o.inl) {
      memcpy(inl, o.inl, sizeof(inl));
      segs = inl;
    } else {
      segs = o.segs;
    }
    o.next = o.seg_end = nullptr;
    o.segs = o.inl;
    o.count = o.entered = o.nseg = 0;
    o.segcap = N_INLINE;
  }

  template<typename F>
  void forEachSegmentMut(F &&f) {
    uint32_t left = count;
    for (uint32_t k = 0; left; k++) {
      const uint32_t n = segElems(k) < left ? segElems(k) : left;
      f(segs[k], n);
      left -= n;
    }
  }

  template<typename H, typename R>
  struct Iter {
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::remove_const_t<R>;
    using difference_type = std::ptrdiff_t;
    using pointer = R *;
    using reference = R &;
    H *h = nullptr;
    uint32_t i = 0;
    R &operator*() const { return (*h)[i]; }
    R *operator->() const { return &(*h)[i]; }
    Iter &operator++() {
      ++i;
      return *this;
    }
    Iter operator++(int) {
      Iter t = *this;
      ++i;
      return t;
    }
    bool operator==(const Iter &o) const { return i == o.i; }
    bool operator!=(const Iter &o) const { return i != o.i; }
  };

public:
  using value_type = TS;
  using size_type = uint32_t;
  using iterator = Iter<SegmentedHistory, TS>;
  using const_iterator = Iter<const SegmentedHistory, const TS>;

  SegmentedHistory() = default;
  SegmentedHistory(const SegmentedHistory &o) {
    o.forEachSegment([this](const TS *p, uint32_t n) {
      for (uint32_t i = 0; i < n; i++)
        emplace_back(p[i]);
    });
  }
  SegmentedHistory(SegmentedHistory &&o) noexcept { stealFrom(o); }
  SegmentedHistory &operator=(const SegmentedHistory &o) {
    if (this != &o) {
      SegmentedHistory t(o);
      *this = std::move(t);
    }
    return *this;
  }
  SegmentedHistory &operator=(SegmentedHistory &&o) noexcept {
    if (this != &o) {
      release();
      stealFrom(o);
    }
    return *this;
  }
  ~SegmentedHistory() { release(); }

  uint32_t size() const { return count; }
  bool empty() const { return count == 0; }
  /// elements that fit without allocating (all allocated segments)
  uint32_t capacity() const {
    uint32_t c = 0;
    for (uint32_t k = 0; k < nseg; k++)
      c += segElems(k);
    return c;
  }

  TS &operator[](uint32_t i) {
    uint32_t k, o;
    locate(i, k, o);
    return segs[k][o];
  }
  const TS &operator[](uint32_t i) const {
    uint32_t k, o;
    locate(i, k, o);
    return segs[k][o];
  }
  TS &front() { return segs[0][0]; }
  const TS &front() const { return segs[0][0]; }
  // next never sits at the start of a segment while count > 0 (see pop_back)
  TS &back() { return next[-1]; }
  const TS &back() const { return next[-1]; }

  TS &push_back() {
    if (next == seg_end) [[unlikely]]
      advance();
    TS *p = next;
    ::new (static_cast<void *>(p)) TS();
    ++next;
    ++count;
    return *p;
  }

  template<typename U>
  TS &emplace_back(U &&v) {
    if (next == seg_end) [[unlikely]]
      advance();
    TS *p = ::new (static_cast<void *>(next)) TS(std::forward<U>(v));
    ++next;
    ++count;
    return *p;
  }

  void pop_back() {
    --next;
    --count;
    next->~TS();
    if (entered > 1 && next == segs[entered - 1]) {
      // keep the empty segment allocated for the next push, step back to the end of the previous one
      entered--;
      seg_end = segs[entered - 1] + segElems(entered - 1);
      next = seg_end;
    }
  }

  /// f(const TS *first, uint32_t n) for each run of contiguous elements, in order
  template<typename F>
  void forEachSegment(F &&f) const {
    uint32_t left = count;
    for (uint32_t k = 0; left; k++) {
      const uint32_t n = segElems(k) < left ? segElems(k) : left;
      f(static_cast<const TS *>(segs[k]), n);
      left -= n;
    }
  }

  iterator begin() { return {this, 0}; }
  iterator end() { return {this, count}; }
  const_iterator begin() const { return {this, 0}; }
  const_iterator end() const { return {this, count}; }
};
