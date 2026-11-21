#pragma once

#include "bitStream.h"

#if defined(_MSC_VER)
#define BITCURSOR_INLINE __forceinline
#elif defined(__GNUC__)
#define BITCURSOR_INLINE inline __attribute__((always_inline))
#else
#define BITCURSOR_INLINE inline
#endif

namespace danet {
  /// A read position over the bytes of a BitStream that lives in locals of the caller.
  ///
  /// BitStream keeps its read offset in the object, and every value it reads is stored through a
  /// uint8_t pointer, which may alias that object: the offset, the size and the data pointer are
  /// loaded again for every read and the offset is stored back. A cursor is a plain local with no
  /// address taken, so all of that stays in registers, and a read is one 8-byte load and two shifts
  /// with no branch on the bit alignment. Keep it that way: every call here is inlined, and a cursor
  /// that is passed by reference to a function that is not inlined is back in memory.
  ///
  /// The calls have the names and the results of the BitStream calls: the same bits end up in the
  /// same bytes, a checked read that does not fit fails without moving the position, and array reads
  /// stop at the first element that does not fit. One thing differs: bits past the last byte of the
  /// stream read as 0. BitStream::ReadBit() reads whatever byte the offset points at.
  ///
  /// Padded = true is for bytes that are followed by at least 8 more bytes that may be loaded (their
  /// values do not matter). Reads then load 8 bytes at the position without looking at how far the
  /// end is.
  template<bool Padded>
  class BitCursorT {
  public:
    BitCursorT(const uint8_t *data_, uint32_t bits_used, uint32_t read_offset = 0) :
      data(data_), bitsUsed(bits_used), readOffset(read_offset) {}
    template<bool P = Padded, typename = eastl::enable_if_t<!P>>
    explicit BitCursorT(const BitStream &bs) : BitCursorT(bs.GetData(), bs.GetNumberOfBitsUsed(), bs.GetReadOffset()) {}

    /// store the position back into the stream the cursor was made from
    void writeBack(const BitStream &bs) const { bs.SetReadOffset(readOffset); }

    uint32_t GetReadOffset() const { return readOffset; }
    void SetReadOffset(uint32_t offs) { readOffset = offs; }
    uint32_t GetNumberOfBitsUsed() const { return bitsUsed; }
    uint32_t GetNumberOfUnreadBits() const { return readOffset < bitsUsed ? bitsUsed - readOffset : 0; }
    const uint8_t *GetData() const { return data; }
    void AlignReadToByteBoundary() { readOffset = (readOffset + 7) & ~7u; }

    BITCURSOR_INLINE bool ReadBit() {
      const uint32_t byte = readOffset >> 3;
      const uint32_t v = byte < usedBytes() ? data[byte] : 0u;
      const bool r = (v & (0x80u >> (readOffset & 7))) != 0;
      readOffset++;
      return r;
    }
    BITCURSOR_INLINE bool Read(bool &v) {
      if (!fits(1))
        return false;
      v = (data[readOffset >> 3] & (0x80u >> (readOffset & 7))) != 0;
      readOffset++;
      return true;
    }

    template<size_t bits>
    BITCURSOR_INLINE bool ReadBits(uint8_t *out) {
      static_assert(bits > 0);
      if (!fits(bits))
        return false;
      readFitting<bits>(out);
      return true;
    }

    BITCURSOR_INLINE bool ReadBits(uint8_t *out, uint32_t bits) {
      if (!bits)
        return true;
      if (!fits(bits))
        return false;
      const uint32_t bytes = bits >> 3;
      const uint32_t rem = bits & 7;
      uint32_t i = 0;
      for (; i + 4 <= bytes; i += 4)
        readFitting<32>(out + i);
      for (; i < bytes; ++i)
        readFitting<8>(out + i);
      if (rem) {
        out[i] = uint8_t(peek() >> (64 - rem));
        readOffset += rem;
      }
      return true;
    }

    template<typename T>
    BITCURSOR_INLINE bool Read(T &t) {
      static_assert(eastl::is_trivially_copyable_v<T> && !eastl::is_pointer_v<T> && !eastl::is_const_v<T> &&
                    !eastl::is_array_v<T>);
      return ReadBits<sizeof(T) * 8>((uint8_t *) &t);
    }
    // as BitStream: one read per element, so a short stream still gives the elements that fit
    template<typename T, size_t asz>
    BITCURSOR_INLINE bool Read(T (&cont)[asz]) {
      if constexpr (!eastl::is_same_v<T, bool> && eastl::is_trivially_copyable_v<T> && !eastl::is_array_v<T>) {
        if (DAGOR_LIKELY(fits(sizeof(T) * 8 * asz))) {
          readFitting<sizeof(T) * 8 * asz>((uint8_t *) &cont[0]);
          return true;
        }
      }
      for (auto &elem: cont)
        if (!Read(elem))
          return false;
      return true;
    }

    BITCURSOR_INLINE bool ReadCompressed(int64_t &v) { return readCompressedSigned(v); }
    BITCURSOR_INLINE bool ReadCompressed(int32_t &v) { return readCompressedSigned(v); }
    BITCURSOR_INLINE bool ReadCompressed(int16_t &v) { return readCompressedSigned(v); }
    BITCURSOR_INLINE bool ReadCompressed(uint64_t &v) { return readCompressedUnsigned(v); }
    BITCURSOR_INLINE bool ReadCompressed(uint32_t &v) { return readCompressedUnsigned(v); }
    BITCURSOR_INLINE bool ReadCompressed(uint16_t &v) { return readCompressedUnsigned(v); }

  private:
    BITCURSOR_INLINE uint32_t usedBytes() const { return (bitsUsed + 7) >> 3; }
    // readOffset <= bitsUsed && bits <= bitsUsed - readOffset, as one compare
    BITCURSOR_INLINE bool fits(uint32_t bits) const { return uint64_t(readOffset) + bits <= bitsUsed; }

    // The stream from the read position on, first bit in the top bit: at least 57 bits of it. Only for a
    // position inside the stream. Without padding, near the end the 8 bytes are taken from the last 8 of
    // the stream and moved down, so that nothing past the end is loaded.
    BITCURSOR_INLINE uint64_t peek() const {
      const uint32_t byte = readOffset >> 3;
      uint64_t w;
      if constexpr (Padded) {
        memcpy(&w, data + byte, 8);
      } else {
        const uint32_t loadBytes = usedBytes();
        if (DAGOR_LIKELY(byte + 8 <= loadBytes)) {
          memcpy(&w, data + byte, 8);
        } else if (DAGOR_LIKELY(loadBytes >= 8)) {
          memcpy(&w, data + loadBytes - 8, 8);
          w >>= (byte + 8 - loadBytes) * 8;
        } else {
          w = loadShort(data, loadBytes, byte);
        }
      }
      return swap::byte_swap(w) << (readOffset & 7);
    }
    // a stream of less than 8 bytes. Static: a call that takes `this` would force the cursor into memory.
#if defined(__GNUC__)
    __attribute__((noinline, cold))
#endif
    static uint64_t loadShort(const uint8_t *data, uint32_t load_bytes, uint32_t byte) {
      uint64_t w = 0;
      if (byte < load_bytes)
        memcpy(&w, data + byte, load_bytes - byte);
      return w;
    }

    // `bits` bits that are known to fit: whole bytes in stream order, then the rest in the low bits of
    // one more byte
    template<size_t bits>
    BITCURSOR_INLINE void readFitting(uint8_t *out) {
      size_t done = 0;
      size_t left = bits;
      for (; left > 56; left -= 32, done += 4) {
        const uint32_t v = swap::byte_swap(uint32_t(peek() >> 32));
        memcpy(out + done, &v, 4);
        readOffset += 32;
      }
      const size_t bytes = left >> 3;
      const size_t rem = left & 7;
      const uint64_t w = peek();
      if (bytes) {
        const uint64_t v = swap::byte_swap(w);
        memcpy(out + done, &v, bytes);
      }
      if (rem)
        out[done + bytes] = uint8_t((w << (bytes * 8)) >> (64 - rem));
      readOffset += uint32_t(left);
    }

    template<typename T>
    BITCURSOR_INLINE bool readCompressedSigned(T &v) {
      typename eastl::make_unsigned<T>::type vu;
      if (!readCompressedUnsigned(vu))
        return false;
      v = static_cast<T>((vu >> 1) ^ (-(vu & 1)));
      return true;
    }
    template<typename T>
    BITCURSOR_INLINE bool readCompressedUnsigned(T &v) {
      v = 0;
      for (int i = 0; i < (int) sizeof(v) + 1; ++i) {
        uint8_t byte = 0;
        if (!Read(byte))
          return false;
        v |= T(byte & ~(1 << 7)) << (i * 7);
        if ((byte & (1 << 7)) == 0)
          break;
      }
      return true;
    }

    const uint8_t *data;
    uint32_t bitsUsed;
    uint32_t readOffset;
  };
  using BitCursor = BitCursorT<false>;
  using PaddedBitCursor = BitCursorT<true>;
} // namespace danet
