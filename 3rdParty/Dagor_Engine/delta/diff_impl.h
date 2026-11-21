// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daNet/bitStream.h>
#include <daNet/daNetTypes.h>


namespace net {

  namespace delta {

    inline danet::BitStream diff_impl(const danet::BitStream &buf0, const danet::BitStream &buf1,
                                      IMemAlloc *allocator) {
      uint32_t size0 = buf0.GetNumberOfBytesUsed();
      uint32_t size1 = buf1.GetNumberOfBytesUsed();
      uint32_t lowerLimit = std::min(size0, size1);
      uint32_t upperLimit = std::max(size0, size1);

      // start from a copy of the longer buffer (its bytes past lowerLimit are the result as they are),
      // then XOR the common part in place: one pass, and no zero fill of the new buffer first
      const danet::BitStream &src = size1 < size0 ? buf0 : buf1;
      const danet::BitStream &other = size1 < size0 ? buf1 : buf0;
      danet::BitStream ret{src.GetData(), upperLimit, true, allocator};
      uint8_t *__restrict out = ret.GetData();
      const uint8_t *__restrict in = other.GetData();
      for (uint32_t i = 0; i < lowerLimit; ++i)
        out[i] ^= in[i];

      ret.SetWriteOffset(BYTES_TO_BITS(upperLimit));
      return ret;
    }

  } // namespace delta

} // namespace net
