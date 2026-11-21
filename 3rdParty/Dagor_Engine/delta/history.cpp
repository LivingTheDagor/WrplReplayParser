
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/history.h>

#include "diff_impl.h"
#include <daNet/delta/rle.h>

namespace net {

  namespace delta {

    void History::store(int packet_num, const danet::BitStream &bin) {
      danet::BitStream &to = pack[basePacketNo(packet_num)];
      to.SetWriteOffset(0);
      to.reserveBits(bin.GetWriteOffset());
      to.SetWriteOffset(bin.GetWriteOffset());
      memcpy(to.GetData(), bin.GetData(), BITS_TO_BYTES(bin.GetWriteOffset()));
    }

    danet::BitStream History::getDiff(int packet_num, const danet::BitStream &new_ver) const {
      return diff_impl(pack[basePacketNo(packet_num)], new_ver, allocator);
    }

    danet::BitStream History::applyPatch(int packet_num, const danet::BitStream &delta) const {
      return diff_impl(pack[basePacketNo(packet_num)], delta, allocator);
    }

    danet::BitStream get_compressed_delta(const danet::BitStream &base_version, const danet::BitStream &new_ver,
                                          IMemAlloc *allocator) {
      danet::BitStream diff = diff_impl(base_version, new_ver, allocator);
      diff.SetWriteOffset(new_ver.GetWriteOffset());
      diff.AlignWriteToByteBoundary();
      danet::BitStream compressed{allocator};
      net::delta::compress(diff, compressed);
      return compressed;
    }

    danet::BitStream apply_compressed_patch(const danet::BitStream &base_version,
                                            const danet::BitStream &compressed_delta, IMemAlloc *allocator) {
      ZoneScoped;
      // the decoded delta only lives until diff_impl has read it: reuse one buffer per thread,
      // so its 10x reserve is not allocated and zero filled for every patch
      static thread_local danet::BitStream delta{};
      net::delta::decompress(compressed_delta, delta);
      danet::BitStream result = diff_impl(base_version, delta, allocator);
      result.SetWriteOffset(delta.GetWriteOffset());
      result.AlignWriteToByteBoundary();
      return result;
    }

    uint32_t apply_compressed_patch_into(const uint8_t *base, uint32_t base_size,
                                         const danet::BitStream &compressed_delta, danet::BitStream &out,
                                         uint32_t padding) {
      ZoneScoped;
      static thread_local danet::BitStream delta{};
      net::delta::decompress(compressed_delta, delta);
      const uint32_t size = delta.GetNumberOfBytesUsed();
      const bool inPlace = out.GetData() == base && base != nullptr;
      out.SetWriteOffset(0);
      out.reserveBits(BYTES_TO_BITS(size + padding));
      if (inPlace)
        base = out.GetData(); // reserveBits may have moved the buffer; the bytes are the same
      const uint32_t common = std::min(base_size, size);
      uint8_t *dst = out.GetData();
      const uint8_t *d = delta.GetData();
      if (inPlace) {
        for (uint32_t i = 0; i < common; ++i)
          dst[i] ^= d[i];
      } else {
        uint8_t *__restrict o = dst;
        const uint8_t *__restrict b = base;
        const uint8_t *__restrict dd = d;
        for (uint32_t i = 0; i < common; ++i)
          o[i] = b[i] ^ dd[i];
      }
      if (size > common)
        memcpy(dst + common, d + common, size - common);
      out.SetWriteOffset(BYTES_TO_BITS(size));
      return size;
    }

    void History::clear() {
      for (int i = 0; i < HIST_MAX; ++i)
        pack[i].Reset();
    }

    bool History::isEmpty(int packet_num) const { return pack[basePacketNo(packet_num)].GetNumberOfBitsUsed() == 0; }

    void History::savePackToStream(danet::BitStream &bs) const {
      for (int i = 0; i < HIST_MAX; ++i)
        bs.Write(pack[i]);
    }

    bool History::readPackFromStream(danet::BitStream &bs) {
      bool isOk = true;
      for (int i = 0; i < HIST_MAX; ++i)
        isOk &= bs.Read(pack[i]);
      return isOk;
    }

    int64_t History::getDebugAllocSize() const {
      int64_t result = 0LL;
      for (int i = 0; i < HIST_MAX; ++i)
        result += BITS_TO_BYTES(pack[i].GetNumberOfAllocatedBits());
      return result;
    }

  } // namespace delta

} // namespace net
