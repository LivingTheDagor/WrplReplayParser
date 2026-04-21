#include "danet/Bitstream.h"
#include "DataBlock.h"
#include "reader.h"

bool BitStream::Read(DataBlock &blk) const
{
  uint32_t bytesToRead = 0;
  if (!ReadCompressed(bytesToRead))
    return false;
  if (!bytesToRead)
    return true;
  AlignReadToByteBoundary();
  if (readOffset + bytes2bits(bytesToRead) > bitsUsed)
    return false;

  // ok look like this cast is ugly as fuck, and it's my fault and I don't feel like fixing it
  BaseReader rdr{(char*)const_cast<uint8_t*>(GetData() + bits2bytes(readOffset)), (int)bytesToRead, false};
  bool ret = false;
  if (blk.loadFromStream(rdr, nullptr, nullptr))
  {
    readOffset += bytes2bits(bytesToRead);
    ret = true;
  }
  return ret;
}