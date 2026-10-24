
#include <idFieldSerializer.h>
#include <cassert>
#include "consts.h"
#include "utils.h"
#include "math/dag_bits.h"


void writeSize(BitStream &to, uint32_t size_in_bits) {
  G_ASSERT(size_in_bits);
  uint8_t hdr = 0;

  switch (size_in_bits) {
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

uint32_t readSize(const BitStream &from) {
  bool ok = true;
  uint8_t hdr = 0;

  ok &= from.ReadBits(&hdr, 3);
  if (!ok)
    return 0;

  switch (hdr) {
    case 1: return 1; // 1 bit
    case 2: return 8; // 1 byte
    case 3: return 16; // 2 bytes
    case 4: return 32; // 4 bytes
    case 5: return 64; // 6 bytes
    case 6: return 96; // 12 bytes
    case 7: return 128; // 16 bytes
  }

  G_ASSERT(hdr == 0);
  uint32_t sizeInBits = 0;
  ok &= from.ReadCompressed(sizeInBits);
  return ok ? sizeInBits : 0;
}

static bool readCompressed32Checked(const BitStream &from, uint32_t &value) {
  value = 0;
  for (unsigned i = 0; i < 5; ++i) {
    uint8_t byte = 0;
    if (!from.Read(byte) || (i == 4 && (byte & 0xf0)))
      return false;
    value |= uint32_t(byte & 0x7f) << (i * 7);
    if (!(byte & 0x80))
      return true;
  }
  return false;
}

static bool readSizeChecked(const BitStream &from, uint32_t &size) {
  uint8_t hdr = 0;
  if (!from.ReadBits(&hdr, 3))
    return false;
  static constexpr uint32_t fixed_sizes[] = {0, 1, 8, 16, 32, 64, 96, 128};
  if (hdr)
    size = fixed_sizes[hdr];
  else if (!readCompressed32Checked(from, size))
    return false;
  return size != 0;
}

IdFieldSerializer32::IdFieldSerializer32() : currWrSz(0), currRdSz(0) { mem_set_0(sizes); }

IdFieldSerializer32::IdFieldSerializer32(const IdFieldSerializer32 &other) :
  currWrSz(other.currWrSz), currRdSz(other.currRdSz) {
  mem_copy_from(sizes, other.sizes.data());
}

IdFieldSerializer32 &IdFieldSerializer32::operator=(const IdFieldSerializer32 &other) {
  currWrSz = other.currWrSz;
  currRdSz = other.currRdSz;
  mem_copy_from(sizes, other.sizes.data());
  return *this;
}

void IdFieldSerializer32::setFieldSize(uint32_t sz) {
  G_ASSERT(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}


void IdFieldSerializer32::checkFieldSize(uint8_t index, BitSize_t sz) const {
  G_ASSERT(index < currRdSz);
  G_ASSERT(sz == sizes[index]);
}

void IdFieldSerializer32::writeFieldsSize(BitStream &to) const {
  G_ASSERT(currWrSz);
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset());
  G_ASSERT(offset <= USHRT_MAX);
  for (uint8_t j = 0; j < currWrSz; ++j) {
    G_ASSERT(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(0);
  to.Write((uint16_t) offset);
  to.SetWriteOffset(end);
}

uint32_t IdFieldSerializer32::readFieldsSizeAndFlag(const BitStream &from) {
  uint32_t fields = 0;
  uint16_t offset = 0;
  BitSize_t start = from.GetReadOffset();
  G_ASSERT((start & 7) == 0); // aligned to byte
  from.Read(offset);
  from.ReadCompressed(fields);
  BitSize_t startBody = from.GetReadOffset();
  from.SetReadOffset(BYTES_TO_BITS(offset) + start);
  currRdSz = static_cast<uint8_t>(__popcount(fields));

  G_ASSERT(currRdSz);
  for (uint8_t j = 0; j < currRdSz; ++j) {
    sizes[j] = readSize(from);
    G_ASSERT(sizes[j]);
  }
  from.SetReadOffset(startBody);
  return fields;
}

bool IdFieldSerializer32::readFieldsSizeAndFlagChecked(const BitStream &from, uint32_t &fields) {
  const BitSize_t start = from.GetReadOffset();
  uint16_t offset = 0;
  fields = 0;
  currRdSz = 0;
  if ((start & 7) || !from.Read(offset) || !readCompressed32Checked(from, fields) || !fields)
    return false;
  const BitSize_t body = from.GetReadOffset();
  const uint64_t table = uint64_t(start) + uint64_t(offset) * 8;
  if (table < body || table > from.GetNumberOfBitsUsed())
    return false;
  from.SetReadOffset(static_cast<BitSize_t>(table));
  currRdSz = static_cast<uint8_t>(__popcount(fields));
  uint64_t body_bits = 0;
  for (uint8_t j = 0; j < currRdSz; ++j) {
    if (!readSizeChecked(from, sizes[j]) || (body_bits += sizes[j]) > table - body)
      return false;
  }
  from.SetReadOffset(body);
  return true;
}

void IdFieldSerializer32::skipReadingField(uint8_t index, const BitStream &from) const {
  G_ASSERT(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}


IdFieldSerializer255::IdFieldSerializer255() : currWrId(0), currWrSz(0), currRdSz(0), bitsPerId(0) {
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::reset() {
  currWrId = 0;
  currWrSz = 0;
  currRdSz = 0;
  bitsPerId = 0;
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::setFieldSize(uint32_t sz) {
  G_ASSERT(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}

void IdFieldSerializer255::setFieldId(Id id) {
  G_ASSERT(currWrId < MAX_FIELDS_NUM);
  indices[currWrId++] = id;
}

void IdFieldSerializer255::writeFieldsIndex(BitStream &to, BitSize_t at) const {
  G_ASSERT(currWrId);
  G_ASSERT((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t endBody = to.GetWriteOffset();
  // we are at the endBody
  G_ASSERT(currWrId == currWrSz);
  to.SetWriteOffset(at + BYTES_TO_BITS(sizeof(uint16_t)) /*offset*/);
  Index maxId = 1; //__bsr(0) === 32
  for (Index j = 0; j < currWrId; ++j) {
    if (indices[j] > maxId)
      maxId = indices[j];
  }
  Index bitsPerIdToWrite = (Index) (std::bit_width(maxId) + 1); // __bsr instead of std::bit_width
  Index countToWrite = (Index) (currWrId | (bitsPerIdToWrite << BITS_PER_COUNT));
  to.Write(countToWrite);
  to.SetWriteOffset(endBody);
  for (Index j = 0; j < currWrId; ++j)
    to.WriteBits(reinterpret_cast<const uint8_t *>(&indices[j]), bitsPerIdToWrite);
}

inline static BitSize_t alingedToByte(BitSize_t count) { return count + 8 - (((count - 1) & 7) + 1); }

bool IdFieldSerializer255::readFieldsIndex(const BitStream &from) {
  if (!currRdSz || !bitsPerId)
    return false;
  BitSize_t startBody = from.GetReadOffset();
  // we are at the startBody so reread offset and check count
  uint16_t offset = 0;
  Index count = 0;
  const BitSize_t rewind = BYTES_TO_BITS(sizeof(offset)) + BYTES_TO_BITS(sizeof(count));
  if (startBody < rewind)
    return false;
  from.SetReadOffset(startBody - rewind);
  if (!from.Read(offset) || !from.Read(count) || count != (currRdSz | (bitsPerId << BITS_PER_COUNT)))
    return false;
  BitSize_t bitsForIndices = alingedToByte(currRdSz * bitsPerId);
  const BitSize_t table = startBody - rewind + BYTES_TO_BITS(offset);
  if (table < startBody + bitsForIndices)
    return false;
  from.SetReadOffset(table - bitsForIndices);
  for (Index j = 0; j < currRdSz; ++j)
    if (!from.ReadBits(reinterpret_cast<uint8_t *>(&indices[j]), bitsPerId))
      return false;
  from.SetReadOffset(startBody);
  return true;
}

IdFieldSerializer255::Id IdFieldSerializer255::getFieldId(Index index) const { return indices[index]; }

uint32_t IdFieldSerializer255::getFieldSize(Index index) const { return sizes[index]; }

void IdFieldSerializer255::writeFieldsSize(BitStream &to, BitSize_t at) const {
  G_ASSERT(currWrId == 0 || currWrId == currWrSz);
  G_ASSERT(currWrSz);
  G_ASSERT((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset() - at);
  G_ASSERT(offset <= USHRT_MAX);
  for (Index j = 0; j < currWrSz; ++j) {
    G_ASSERT(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(at);
  to.Write((uint16_t) offset);
  to.SetWriteOffset(end);
}

// Returns 0 for a malformed table. The count has 12 bits but sizes holds MAX_FIELDS_NUM entries, and
// callers keep per-field arrays of that size, so a larger count must not get through.
IdFieldSerializer255::Index IdFieldSerializer255::readFieldsSizeAndCount(const BitStream &from, BitSize_t &end) {
  uint16_t offset = 0;
  Index count = 0;
  BitSize_t start = from.GetReadOffset();
  end = start;
  currRdSz = 0;
  if ((start & 7) || !from.Read(offset) || !from.Read(count))
    return 0;
  BitSize_t startBody = from.GetReadOffset();
  const uint64_t table = uint64_t(start) + BYTES_TO_BITS(uint64_t(offset));
  const Index fields = count & BIT_MASK_COUNT;
  if (!fields || fields > MAX_FIELDS_NUM || table < startBody || table > from.GetNumberOfBitsUsed())
    return 0;
  from.SetReadOffset(static_cast<BitSize_t>(table));
  uint64_t body_bits = 0;
  for (Index j = 0; j < fields; ++j) {
    if (!readSizeChecked(from, sizes[j]) || (body_bits += sizes[j]) > table - startBody)
      return 0;
  }
  from.AlignReadToByteBoundary();
  if (from.GetReadOffset() > from.GetNumberOfBitsUsed())
    return 0;
  end = from.GetReadOffset();
  from.SetReadOffset(startBody);
  currRdSz = fields;
  bitsPerId = count >> BITS_PER_COUNT;
  return currRdSz;
}

void IdFieldSerializer255::skipReadingField(Index index, const BitStream &from) const {
  G_ASSERT(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}
