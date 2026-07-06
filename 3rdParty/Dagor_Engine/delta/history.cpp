
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/history.h>

#include "diff_impl.h"
#include <daNet/delta/rle.h>

namespace net
{

  namespace delta
  {

    void History::store(int packet_num, const BitStream &bin)
    {
      BitStream &to = pack[basePacketNo(packet_num)];
      to.SetWriteOffset(0);
      to.reserveBits(bin.GetWriteOffset());
      to.SetWriteOffset(bin.GetWriteOffset());
      memcpy(to.GetData(), bin.GetData(), BITS_TO_BYTES(bin.GetWriteOffset()));
    }

    BitStream History::getDiff(int packet_num, const BitStream &new_ver) const
    {
      return diff_impl(pack[basePacketNo(packet_num)], new_ver, allocator);
    }

    BitStream History::applyPatch(int packet_num, const BitStream &delta) const
    {
      return diff_impl(pack[basePacketNo(packet_num)], delta, allocator);
    }

    BitStream get_compressed_delta(const BitStream &base_version, const BitStream &new_ver, IMemAlloc *allocator)
    {
      BitStream diff = diff_impl(base_version, new_ver, allocator);
      diff.SetWriteOffset(new_ver.GetWriteOffset());
      diff.AlignWriteToByteBoundary();
      BitStream compressed{allocator};
      net::delta::compress(diff, compressed);
      return compressed;
    }

    BitStream apply_compressed_patch(const BitStream &base_version, const BitStream &compressed_delta,
                                     IMemAlloc *allocator)
    {
      ZoneScoped;
      BitStream delta{allocator};
      net::delta::decompress(compressed_delta, delta);
      BitStream result = diff_impl(base_version, delta, allocator);
      result.SetWriteOffset(delta.GetWriteOffset());
      result.AlignWriteToByteBoundary();
      return result;
    }

    void History::clear()
    {
      for (int i = 0; i < HIST_MAX; ++i)
        pack[i].Reset();
    }

    bool History::isEmpty(int packet_num) const { return pack[basePacketNo(packet_num)].GetNumberOfBitsUsed() == 0; }

    void History::savePackToStream(BitStream &bs) const
    {
      for (int i = 0; i < HIST_MAX; ++i)
        bs.Write(pack[i]);
    }

    bool History::readPackFromStream(BitStream &bs)
    {
      bool isOk = true;
      for (int i = 0; i < HIST_MAX; ++i)
        isOk &= bs.Read(pack[i]);
      return isOk;
    }

    int64_t History::getDebugAllocSize() const
    {
      int64_t result = 0LL;
      for (int i = 0; i < HIST_MAX; ++i)
        result += BITS_TO_BYTES(pack[i].GetNumberOfAllocatedBits());
      return result;
    }

  } // namespace delta

} // namespace net