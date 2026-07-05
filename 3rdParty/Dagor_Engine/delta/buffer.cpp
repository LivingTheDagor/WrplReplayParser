
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/buffer.h>

#include "diff_impl.h"

namespace net
{

  namespace delta
  {
    const uint8_t versionMask = 0x7f;
    const uint8_t versionIncrement = 0x01;
    const uint8_t fullVersionMask = 0x80;

    Buffer::Buffer(IMemAlloc * allocator) : allocator(allocator), version(0), confirmedVersion(0) { reset(); }

    void Buffer::reset()
    {
      base.Reset();
      incrementVersion();
      setFullVersion();
      confirmedVersion = ~version;
    }

    void Buffer::incrementVersion() { version = ((version & versionMask) + versionIncrement) & versionMask; }

    void Buffer::setFullVersion() { version = (version & versionMask) | fullVersionMask; }

    void Buffer::setBase(const BitStream &data)
    {
      base = data;
      incrementVersion();
    }

    void Buffer::setBase(BitStream &&data)
    {
      base = std::move(data);
      incrementVersion();
    }

    BitStream Buffer::getDiff(const BitStream &new_ver) const { return diff_impl(base, new_ver, allocator); }

    BitStream Buffer::applyPatch(const BitStream &delta)
    {
      BitStream result = diff_impl(base, delta, allocator);
      setBase(result);
      return result;
    }

    bool Buffer::checkVersionAndNeedApply(uint8_t incomming_version)
    {
      if (incomming_version & fullVersionMask)
      {
        reset();
        version = incomming_version;
        return true;
      }
      return incomming_version == version;
    }

  } // namespace delta

} // namespace net