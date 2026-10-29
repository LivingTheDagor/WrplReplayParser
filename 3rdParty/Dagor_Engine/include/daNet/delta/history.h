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

  } // namespace delta

} // namespace net