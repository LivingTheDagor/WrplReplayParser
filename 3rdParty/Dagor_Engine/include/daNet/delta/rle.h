//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daNet/bitStream.h>
#include "span"

namespace net
{

  namespace delta
  {

    void compress(const BitStream &in, BitStream &out);
    inline BitStream compress(const BitStream &in)
    {
      BitStream out;
      compress(in, out);
      return out;
    }

    void decompress(const BitStream &in, BitStream &out);
    inline BitStream decompress(const BitStream &in, IMemAlloc * allocator)
    {
      BitStream out(allocator);
      decompress(in, out);
      return out;
    }

// raw interface
    int rle0ki_compress(std::span<uint8_t> dstSlice, std::span<uint8_t> srcSlice);
    int rle0ki_decompress(std::span<uint8_t> dstSlice, std::span<uint8_t> srcSlice);

  } // namespace delta

} // namespace net