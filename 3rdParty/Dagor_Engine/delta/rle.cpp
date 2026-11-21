
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/rle.h>
#include <bit>
#include <cstring>
#include <array>
#include <dag_assert.h>
#include <daNet/daNetTypes.h>
#include "Logger.h"


namespace net::delta {

  static const int TWO_BITS_SHIFT = 2;
  static const int TWO_BITS_MASK = 3;
  static const int SIX_BITS_MASK = 63;
  static const int ZR0_BASE_VAL = 3;
  static const int ZR1_BASE_VAL = 7;
  static const int MIN_UNITS_NUM = 3;
  static const int MAX_ZEROS_NUM = 10;

  static const int RLE0_PARITY_NO = 111;
  static const int RLE0_PARITY_YES = 222;
  static const int RLE0_ZERO = 0;
  static const int RLE0_FOUR = 4;
  static const int RLE0_ZR0 = 0;
  static const int RLE0_ZR1 = 3;
  static const int RLE0_KI2 = 2;
  static const int RLE0_KI6 = 1;
  static const int RLE0_KI8 = 4;

  //
  // this is rather bit algorithm than byte one
  // that allows to work on relatively short data about 50 bytes
  //
  // the operating unit size is two bits
  // the main idea is to compress zeros only so it requires many zeros about 50-80%
  //
  // such amount of zeros is considered to be got as result of xor'ed diff
  //
  // the number of units is coded into other two bits
  // so we have 4 bit codes:
  //[zr0, N] for sequence of 3 - 6 zero units
  //[zr1, N] for sequence of 7 - 10 zero units
  //
  // other unit values are kept intact
  // so we have 8 bit code for 6 bits of data:
  //[ki6, unit1, unit2, unit3] for sequence of 3 non-zero units
  // and 4 bit code for 2 bits of data:
  //[ki2, unit1] mostly for remainder units
  //
  // best regular compression is 5:1
  //
  int rle0ki_compress(std::span<uint8_t> dstSlice, std::span<uint8_t> srcSlice) {
    uint8_t *dst = dstSlice.data();
    int maxDstSize = dstSlice.size();
    const uint8_t *src = srcSlice.data();
    int srcSize = srcSlice.size();

    int accumVal = 0;
    int accumLen = 0;
    int zeroCount = 0;

    if (srcSize == 0)
      return 0;
    if (srcSize < 0)
      return -1;

    int val = 0;

    int offset = 0;
    uint8_t *out = dst;
    if (maxDstSize > 0)
      *dst = 0;
    else
      return -1;

    for (int i = 0; i < srcSize; ++i) {
      for (int k = 0; k < RLE0_FOUR; ++k) {
        int next = (*src >> (TWO_BITS_SHIFT * k)) & TWO_BITS_MASK;

        if (accumLen >= MIN_UNITS_NUM * TWO_BITS_SHIFT) {
          int num = 0;
          int data = 0;
          int current = accumVal & SIX_BITS_MASK;

          switch (current) {
            // serie
            case 0: // 00 00 00

              if (next != val || zeroCount == MAX_ZEROS_NUM) {
                data = zeroCount < ZR1_BASE_VAL ? RLE0_ZR0 : RLE0_ZR1;
                data |= (zeroCount - (data == RLE0_ZR0 ? ZR0_BASE_VAL : ZR1_BASE_VAL)) << TWO_BITS_SHIFT;
                num = 1;
                accumVal >>= zeroCount * TWO_BITS_SHIFT;
                accumLen -= zeroCount * TWO_BITS_SHIFT;
                zeroCount = 0;
              }
              break;

              // ki2
            case 1: // 00 00 01
            case 2: // 00 00 10
            case 3: // 00 00 11

              if (next == val) {
                data = RLE0_KI2 | current << TWO_BITS_SHIFT;
                num = 1;
                accumVal >>= TWO_BITS_SHIFT;
                accumLen -= TWO_BITS_SHIFT;
                break;
              }
              [[fallthrough]];

              // ki6
            default:

              data = RLE0_KI6 | current << TWO_BITS_SHIFT;
              num = 2;
              accumVal = 0;
              accumLen = 0;
              zeroCount = 0;
          }

          if (num) {
            *dst |= data << offset;
            if (offset == RLE0_FOUR || num == 2) {
              if (++dst - out < maxDstSize)
                *dst = offset == RLE0_FOUR && num == 2 ? data >> RLE0_FOUR : 0;
            }
            if (num == 1)
              offset = RLE0_FOUR - offset;
          }
        }
        // add to accumulator
        val = next;
        accumVal |= val << accumLen;
        accumLen += TWO_BITS_SHIFT;
        zeroCount += !val;
      }
      ++src;
    }

    // last iteration similar to the used in the cycle above
    int pairs = accumLen / TWO_BITS_SHIFT;
    if (pairs >= MIN_UNITS_NUM) {
      int num = 0;
      int data = 0;
      int current = accumVal & SIX_BITS_MASK;

      if (current) {
        data = RLE0_KI6 | current << TWO_BITS_SHIFT;
        num = 2;
      } else {
        data = zeroCount < ZR1_BASE_VAL ? RLE0_ZR0 : RLE0_ZR1;
        data |= (zeroCount - (data == RLE0_ZR0 ? ZR0_BASE_VAL : ZR1_BASE_VAL)) << TWO_BITS_SHIFT;
        num = 1;
      }

      *dst |= data << offset;
      if (offset == RLE0_FOUR || num == 2) {
        if (++dst - out < maxDstSize)
          *dst = offset == RLE0_FOUR && num == 2 ? data >> RLE0_FOUR : 0;
      }
      if (num == 1)
        offset = RLE0_FOUR - offset;
    } else // remainder
    {
      for (int n = 0; n < pairs; ++n) {
        val = accumVal & TWO_BITS_MASK;
        accumVal >>= TWO_BITS_SHIFT;
        *dst |= (RLE0_KI2 << offset) | (val << (TWO_BITS_SHIFT + offset));
        if (offset == RLE0_FOUR) {
          if (++dst - out < maxDstSize)
            *dst = 0;
        }
        offset = RLE0_FOUR - offset;
      }
    }
    if (offset)
      ++dst;
    if (dst - out >= maxDstSize)
      return -1;

    *dst++ = offset == RLE0_ZERO ? RLE0_PARITY_YES : RLE0_PARITY_NO;
    if (dst - out > maxDstSize)
      return -1;

    return dst - out;
  }


  namespace {
    // One step of the rle0ki decoder per 4-bit code: what it appends (value, bit count) and the next
    // state. State 0 is "normal", states 1..4 are "inside KI8" with super = state - 1. Values are
    // at most 4 bits (two units, the first in the low bits); a zero run is value 0 with its length.
    struct Rle0Step {
      uint8_t value;
      uint8_t bits;
      uint8_t next;
    };
    constexpr std::array<Rle0Step, 5 * 16> make_rle0_steps() {
      std::array<Rle0Step, 5 * 16> t{};
      for (int state = 0; state < 5; ++state)
        for (int nib = 0; nib < 16; ++nib) {
          const int v0 = nib & TWO_BITS_MASK;
          const int v1 = (nib >> TWO_BITS_SHIFT) & TWO_BITS_MASK;
          Rle0Step &e = t[state * 16 + nib];
          if (state == 0) {
            switch (v0) {
              case RLE0_ZR0: e = {0, uint8_t(2 * (v1 + ZR0_BASE_VAL)), 0}; break;
              case RLE0_ZR1: e = {0, uint8_t(2 * (v1 + ZR1_BASE_VAL)), 0}; break;
              case RLE0_KI6: e = {uint8_t(v1), 2, uint8_t(1 + v1)}; break;
              case RLE0_KI2: e = {uint8_t(v1), 2, 0}; break;
            }
          } else {
            const int super = state - 1;
            if (super == 0 && v0 == 0 && v1 == 0)
              e = {0, 2 * 39, 0};
            else
              e = {uint8_t(v0 | (v1 << 2)), 4, 0};
          }
        }
      return t;
    }
    constexpr std::array<Rle0Step, 5 * 16> RLE0_STEPS = make_rle0_steps();
  } // namespace

  // Table-driven form of the decoder that was here before (same output and the same -1 cases):
  // each 4-bit code is one table lookup instead of a switch with a lambda per unit.
  int rle0ki_decompress(std::span<uint8_t> dstSlice, std::span<uint8_t> srcSlice) {
    ZoneScoped;
    uint8_t *dst = dstSlice.data();
    const size_t maxOriginalSize = dstSlice.size();
    const uint8_t *src = srcSlice.data();
    const size_t compressedSize = srcSlice.size();

    if (compressedSize == 0)
      return 0;
    const int limit = (int) compressedSize - 1;

    const uint8_t parity = src[limit];
    if (parity != RLE0_PARITY_NO && parity != RLE0_PARITY_YES)
      return 0;
    if (maxOriginalSize == 0)
      return -1;

    uint64_t acc = 0; // pending bits, low bits first
    uint32_t nbits = 0; // bits pending in acc, always < 64 between steps
    size_t outBytes = 0;
    bool overflow = false;
    uint32_t state = 0;

    auto flush8 = [&]() {
      if (outBytes + 8 <= maxOriginalSize) {
        if constexpr (std::endian::native == std::endian::little)
          memcpy(dst + outBytes, &acc, 8);
        else
          for (int b = 0; b < 8; ++b)
            dst[outBytes + b] = uint8_t(acc >> (8 * b));
      } else {
        overflow = true;
      }
      outBytes += 8;
    };
    auto step = [&](uint32_t nib) {
      const Rle0Step e = RLE0_STEPS[state * 16 + nib];
      state = e.next;
      acc |= uint64_t(e.value) << nbits;
      const uint32_t before = nbits;
      nbits += e.bits;
      if (DAGOR_UNLIKELY(nbits >= 64)) {
        flush8();
        // what did not fit: the high units of a 2-unit value (nbits is even, so at most one unit),
        // and zeros for the rest of a zero run
        acc = before ? uint64_t(e.value) >> (64 - before) : 0;
        nbits -= 64;
        while (nbits >= 64) {
          flush8();
          acc = 0;
          nbits -= 64;
        }
      }
    };

    for (int i = 0; i < limit - 1; ++i) {
      const uint8_t byte = src[i];
      step(byte & 15);
      step(byte >> 4);
    }
    if (limit > 0) {
      const uint8_t byte = src[limit - 1];
      step(byte & 15);
      if (parity == RLE0_PARITY_YES)
        step(byte >> 4);
    }

    const size_t tail = (nbits + 7) / 8;
    if (overflow || outBytes + tail > maxOriginalSize)
      return -1;
    for (size_t b = 0; b < tail; ++b)
      dst[outBytes + b] = uint8_t(acc >> (8 * b));
    return (int) (outBytes + tail);
  }

  int rle0ki_decompress_old(std::span<uint8_t> dstSlice, std::span<uint8_t> srcSlice) {
    ZoneScoped;
    uint8_t *dst = dstSlice.data();
    size_t maxOriginalSize = dstSlice.size();
    const uint8_t *src = srcSlice.data();
    size_t compressedSize = srcSlice.size();

    if (compressedSize == 0)
      return 0;
    int limit = compressedSize - 1;
    if (limit < 0)
      return -1;

    uint8_t parity = src[limit];
    if (parity != RLE0_PARITY_NO && parity != RLE0_PARITY_YES)
      return 0;

    int val[] = {0, 0};
    int code = -1;
    int offset = RLE0_ZERO;
    uint8_t *out = dst;
    if (maxOriginalSize > 0)
      *dst = 0;
    else
      return -1;

    int super = -1;
    for (int i = 0; i < limit; ++i) {
      int klim = i != limit - 1 || parity == RLE0_PARITY_YES ? 2 : 1;
      for (int k = 0; k < klim; ++k) {
        val[0] = (*src >> (k * RLE0_FOUR)) & TWO_BITS_MASK;
        val[1] = (*src >> (TWO_BITS_SHIFT + k * RLE0_FOUR)) & TWO_BITS_MASK;
        int num = 0;

        if (code != RLE0_KI8)
          code = val[0];

        switch (code) {
          case RLE0_ZR0:
            num = val[1] + ZR0_BASE_VAL;
            val[0] = val[1] = 0;
            break;

          case RLE0_ZR1:
            num = val[1] + ZR1_BASE_VAL;
            val[0] = val[1] = 0;
            break;

          case RLE0_KI6:
            code = RLE0_KI8;
            super = val[1];
            [[fallthrough]];

          case RLE0_KI2:
            num = 1;
            val[0] = val[1];
            break;

          case RLE0_KI8:
            code = -1;
            num = 2;
            if (super == 0 && super == val[0] && val[0] == val[1])
              num = 39;
            break;
        }

        for (int n = 0; n < num; ++n) {
          *dst |= val[n & 1] << (offset * TWO_BITS_SHIFT);
          ++offset;
          if (offset == RLE0_FOUR) {
            offset = RLE0_ZERO;
            ++dst;
            int len = dst - out;
            if (len < maxOriginalSize)
              *dst = 0;
            else if (len > maxOriginalSize)
              return -1;
          }
        }
      }
      ++src;
    }

    if (offset)
      ++dst;
    if (dst - out > maxOriginalSize)
      return -1;
    return dst - out;
  }

  void compress(const danet::BitStream &in, danet::BitStream &out) {
    int maxSize = in.GetNumberOfBytesUsed() * 2; // compression can't be worse than 1:2
    out.SetWriteOffset(0);
    out.reserveBits(BYTES_TO_BITS(maxSize));
    int len = -1;
    std::span<uint8_t> dest(out.GetData(), maxSize);
    len = rle0ki_compress(dest, in.getSlice());
    if (len >= 0)
      out.SetWriteOffset(BYTES_TO_BITS(len));
    else {
      LOGE("rle0ki_compress has been failed on a stream of size %d", in.GetNumberOfBytesUsed());

      danet::BitStream empty;
      out.swap(empty);
    }
  }

  void decompress(const danet::BitStream &in, danet::BitStream &out) {
    int maxSize = in.GetNumberOfBytesUsed() * 10; // compression can't be better than 10:1
    out.SetWriteOffset(0);
    out.reserveBits(BYTES_TO_BITS(maxSize));
    int len = -1;
    std::span<uint8_t> dest(out.GetData(), maxSize);
    len = rle0ki_decompress(dest, in.getSlice());
    G_ASSERT(len != -1);
    out.SetWriteOffset(BYTES_TO_BITS(len));
  }

} // namespace net::delta
