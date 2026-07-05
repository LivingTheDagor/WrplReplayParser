//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daNet/bitStream.h>

namespace net
{

  namespace delta
  {

    class Buffer
    {
      void incrementVersion();
      void setFullVersion();

    public:
      Buffer(IMemAlloc * allocator);

      void reset();

      uint8_t getVersion() const { return version; }
      uint8_t getConfirmedVersion() const { return confirmedVersion; }
      void setConfirmedVersion(uint8_t vers) { confirmedVersion = vers; }

      void setBase(const BitStream &data);
      void setBase(BitStream &&data);

      BitStream getDiff(const BitStream &new_ver) const;
      BitStream applyPatch(const BitStream &delta);

      // check version: full packet make reset and need apply, different version skip - return false.
      bool checkVersionAndNeedApply(uint8_t incomming_version);

    private:
      BitStream base;
      IMemAlloc * allocator;
      uint8_t version;
      uint8_t confirmedVersion;
    };

  } // namespace delta

} // namespace net