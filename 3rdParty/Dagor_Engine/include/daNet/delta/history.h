//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daNet/bitStream.h>
#include <dag_assert.h>

namespace net
{

  namespace delta
  {

    class History
    {
      template <int N>
      struct PowerOfTwo
      {
        static constexpr int val = N && !(N & (N - 1));
      };

      static const int HIST_MAX = 256;

    public:
      template <int N = HIST_MAX>
      static uint8_t basePacketNo(int packet_num)
      {
        G_STATIC_ASSERT(PowerOfTwo<N>::val);
        return packet_num & (N - 1);
      }

      void store(int packet_num, const danet::BitStream &bin);

      danet::BitStream getDiff(int packet_num, const danet::BitStream &new_ver) const;
      danet::BitStream applyPatch(int packet_num, const danet::BitStream &delta) const;

      void clear();
      bool isEmpty(int packet_num) const;

      int64_t getDebugAllocSize() const;

      void savePackToStream(danet::BitStream &bs) const;
      bool readPackFromStream(danet::BitStream &bs);

      History(IMemAlloc * allocator) : allocator(allocator) {}

    private:
      std::array<danet::BitStream, HIST_MAX> pack;
      IMemAlloc * allocator = nullptr;
    };

    danet::BitStream get_compressed_delta(const danet::BitStream &base_version, const danet::BitStream &new_ver, IMemAlloc *allocator);
    danet::BitStream apply_compressed_patch(const danet::BitStream &base_version, const danet::BitStream &compressed_delta,
                                     IMemAlloc *allocator);
    /// apply_compressed_patch into `out`, reusing its buffer. `base`/`base_size` are the base version's bytes;
    /// `out` may own `base` (patch in place). Returns the result size in bytes; the buffer of `out` gets
    /// `padding` more bytes, with no defined value.
    uint32_t apply_compressed_patch_into(const uint8_t *base, uint32_t base_size, const danet::BitStream &compressed_delta,
                                         danet::BitStream &out, uint32_t padding = 0);
    /// apply_compressed_patch_into for a delta that is unpacked already: `delta`/`delta_size` are the bytes
    /// net::delta::decompress gives for the compressed delta.
    uint32_t apply_patch_into(const uint8_t *base, uint32_t base_size, const uint8_t *delta, uint32_t delta_size,
                              danet::BitStream &out, uint32_t padding = 0);

  } // namespace delta

} // namespace net