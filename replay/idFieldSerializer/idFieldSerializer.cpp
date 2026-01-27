
#include <idFieldSerializer.h>
#include <cassert>
#include "consts.h"
#include "utils.h"



void writeSize(BitStream &to, uint32_t size_in_bits)
{
  G_ASSERT(size_in_bits);
  uint8_t hdr = 0;

  switch (size_in_bits)
  {
    case 1:
      hdr = 1; // 1 bit
      break;
    case 8:
      hdr = 2; // 1 byte
      break;
    case 16:
      hdr = 3; // 2 bytes
      break;
    case 32:
      hdr = 4; // 4 bytes
      break;
    case 64:
      hdr = 5; // 8 bytes
      break;
    case 96:
      hdr = 6; // 12 bytes
      break;
    case 128:
      hdr = 7; // 16 bytes
      break;
    default: hdr = 0; // var int bits
  }

  to.WriteBits(&hdr, 3);
  if (hdr > 0)
    return;

  to.WriteCompressed(size_in_bits);
}

uint32_t readSize(const BitStream &from)
{
  bool ok = true;
  uint8_t hdr = 0;

  ok &= from.ReadBits(&hdr, 3);
  if (!ok)
    return 0;

  switch (hdr)
  {
    case 1: return 1;   // 1 bit
    case 2: return 8;   // 1 byte
    case 3: return 16;  // 2 bytes
    case 4: return 32;  // 4 bytes
    case 5: return 64;  // 6 bytes
    case 6: return 96;  // 12 bytes
    case 7: return 128; // 16 bytes
  }

  G_ASSERT(hdr == 0);
  uint32_t sizeInBits = 0;
  ok &= from.ReadCompressed(sizeInBits);
  return ok ? sizeInBits : 0;
}

IdFieldSerializer32::IdFieldSerializer32() : currWrSz(0), currRdSz(0) { mem_set_0(sizes); }

IdFieldSerializer32::IdFieldSerializer32(const IdFieldSerializer32 &other) : currWrSz(other.currWrSz), currRdSz(other.currRdSz)
{
  mem_copy_from(sizes, other.sizes.data());
}

IdFieldSerializer32 &IdFieldSerializer32::operator=(const IdFieldSerializer32 &other)
{
  currWrSz = other.currWrSz;
  currRdSz = other.currRdSz;
  mem_copy_from(sizes, other.sizes.data());
  return *this;
}

void IdFieldSerializer32::setFieldSize(uint32_t sz)
{
  assert(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}


void IdFieldSerializer32::checkFieldSize(uint8_t index, BitSize_t sz) const
{
  assert(index < currRdSz);
  assert(sz == sizes[index]);
}

void IdFieldSerializer32::writeFieldsSize(BitStream &to) const
{
  assert(currWrSz);
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset());
  assert(offset <= USHRT_MAX);
  for (uint8_t j = 0; j < currWrSz; ++j)
  {
    assert(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(0);
  to.Write((uint16_t)offset);
  to.SetWriteOffset(end);
}

uint32_t IdFieldSerializer32::readFieldsSizeAndFlag(const BitStream &from)
{
  uint32_t fields = 0;
  uint16_t offset = 0;
  BitSize_t start = from.GetReadOffset();
  assert((start & 7) == 0); // aligned to byte
  from.Read(offset);
  from.ReadCompressed(fields);
  BitSize_t startBody = from.GetReadOffset();
  from.SetReadOffset(BYTES_TO_BITS(offset) + start);
  currRdSz = static_cast<uint8_t>(popcount(fields));

  assert(currRdSz);
  for (uint8_t j = 0; j < currRdSz; ++j)
  {
    sizes[j] = readSize(from);
    assert(sizes[j]);
  }
  from.SetReadOffset(startBody);
  return fields;
}

void IdFieldSerializer32::skipReadingField(uint8_t index, const BitStream &from) const
{
  assert(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}


IdFieldSerializer255::IdFieldSerializer255() : currWrId(0), currWrSz(0), currRdSz(0), bitsPerId(0)
{
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::reset()
{
  currWrId = 0;
  currWrSz = 0;
  currRdSz = 0;
  bitsPerId = 0;
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::setFieldSize(uint32_t sz)
{
  assert(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}

void IdFieldSerializer255::setFieldId(Id id)
{
  assert(currWrId < MAX_FIELDS_NUM);
  indices[currWrId++] = id;
}

void IdFieldSerializer255::writeFieldsIndex(BitStream &to, BitSize_t at) const
{
  assert(currWrId);
  assert((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t endBody = to.GetWriteOffset();
  // we are at the endBody
  assert(currWrId == currWrSz);
  to.SetWriteOffset(at + BYTES_TO_BITS(sizeof(uint16_t)) /*offset*/);
  Index maxId = 1; //__bsr(0) === 32
  for (Index j = 0; j < currWrId; ++j)
  {
    if (indices[j] > maxId)
      maxId = indices[j];
  }
  Index bitsPerIdToWrite = (Index)(std::bit_width(maxId) + 1); // __bsr instead of std::bit_width
  Index countToWrite = (Index)(currWrId | (bitsPerIdToWrite << BITS_PER_COUNT));
  to.Write(countToWrite);
  to.SetWriteOffset(endBody);
  for (Index j = 0; j < currWrId; ++j)
    to.WriteBits(reinterpret_cast<const uint8_t *>(&indices[j]), bitsPerIdToWrite);
}

inline static BitSize_t alingedToByte(BitSize_t count) { return count + 8 - (((count - 1) & 7) + 1); }

bool IdFieldSerializer255::readFieldsIndex(const BitStream &from)
{
  assert(currRdSz && bitsPerId);
  BitSize_t startBody = from.GetReadOffset();
  // we are at the startBody so reread offset and check count
  uint16_t offset = 0;
  Index count = 0;
  const BitSize_t rewind = BYTES_TO_BITS(sizeof(offset)) + BYTES_TO_BITS(sizeof(count));
  assert(startBody >= rewind);
  from.SetReadOffset(startBody - rewind);
  from.Read(offset);
  from.Read(count);
  assert(count == (currRdSz | (bitsPerId << BITS_PER_COUNT)));
  BitSize_t bitsForIndices = alingedToByte(currRdSz * bitsPerId);
  from.SetReadOffset(startBody - rewind + BYTES_TO_BITS(offset) - bitsForIndices);
  for (Index j = 0; j < currRdSz; ++j)
    if (!from.ReadBits(reinterpret_cast<uint8_t *>(&indices[j]), bitsPerId))
    {
      assert(false);
      return false;
    }
  from.SetReadOffset(startBody);
  return true;
}

IdFieldSerializer255::Id IdFieldSerializer255::getFieldId(Index index) const { return indices[index]; }

uint32_t IdFieldSerializer255::getFieldSize(Index index) const { return sizes[index]; }

void IdFieldSerializer255::writeFieldsSize(BitStream &to, BitSize_t at) const
{
  assert(currWrId == 0 || currWrId == currWrSz);
  assert(currWrSz);
  assert((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset() - at);
  assert(offset <= USHRT_MAX);
  for (Index j = 0; j < currWrSz; ++j)
  {
    assert(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(at);
  to.Write((uint16_t)offset);
  to.SetWriteOffset(end);
}

IdFieldSerializer255::Index IdFieldSerializer255::readFieldsSizeAndCount(const BitStream &from, BitSize_t &end)
{
  uint16_t offset = 0;
  Index count = 0;
  BitSize_t start = from.GetReadOffset();
  assert((start & 7) == 0); // aligned to byte
  from.Read(offset);
  from.Read(count);
  BitSize_t startBody = from.GetReadOffset();
  from.SetReadOffset(BYTES_TO_BITS(offset) + start);
  currRdSz = count & BIT_MASK_COUNT;
  bitsPerId = count >> BITS_PER_COUNT;
  assert(currRdSz);
  for (Index j = 0; j < currRdSz; ++j)
  {
    sizes[j] = readSize(from);
    assert(sizes[j]);
  }
  from.AlignReadToByteBoundary();
  end = from.GetReadOffset();
  from.SetReadOffset(startBody);
  return currRdSz;
}

void IdFieldSerializer255::skipReadingField(Index index, const BitStream &from) const
{
  assert(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}
