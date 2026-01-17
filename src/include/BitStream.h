
#ifndef WTFILEUTILS_BITSTREAM_H
#define WTFILEUTILS_BITSTREAM_H

#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <string>


class BitStream {
public:
  BitStream() {
    this->bitsUsed = 0;
    this->dataOwner = 0;
    this->bitsAllocated = 0;
    this->readOffset = 0;
    this->data = nullptr;
  }

  BitStream(const uint8_t *_data, size_t lenInBytes, bool copy) :
      bitsUsed((uint32_t) bytes2bits(lenInBytes)), dataOwner((uint32_t) copy ? 1 : 0), readOffset(0) {
    if (copy) {
      data = (uint8_t *) malloc(lenInBytes);
      memcpy(data, _data, lenInBytes);
      bitsAllocated = (uint32_t) bytes2bits(lenInBytes);
    } else {
      data = (uint8_t *) _data;
      bitsAllocated = (uint32_t) bytes2bits(lenInBytes);
    }
  }

  BitStream(const char *_data, size_t lenInBytes, bool copy) : BitStream((uint8_t *) _data, lenInBytes, copy) {
  }

  explicit BitStream(uint32_t reserveBytes) : bitsUsed(0), dataOwner(1), readOffset(0) {
    this->data = (uint8_t *)malloc(reserveBytes);
    bitsAllocated = (uint32_t)bytes2bits(reserveBytes);
    memset(data, 0, reserveBytes);
  }

  ~BitStream() {
    if (dataOwner)
      free(GetData());
  }

  void Reset() { bitsUsed = readOffset = 0u; }

  uint32_t GetNumberOfBitsUsed() const { return GetWriteOffset(); }

  uint32_t GetWriteOffset() const { return bitsUsed; }

  uint32_t GetNumberOfBytesUsed(void) const { return bits2bytes(bitsUsed); }

  uint32_t GetReadOffset() const { return readOffset; }

  void SetReadOffset(uint32_t newReadOffset) const { readOffset = newReadOffset; }

  uint32_t GetNumberOfUnreadBits() const { return bitsUsed - readOffset; }

  uint32_t GetNumberOfAllocatedBits() const { return bitsAllocated; }

  uint8_t *GetData() { return data; }

  const uint8_t *GetData() const { return data; }

  const uint8_t *GetDataOffset() { return data + bits2bytes(readOffset); }

  void IgnoreBits(uint32_t bits) const { readOffset += std::min(bits, bitsUsed - readOffset); }

  void IgnoreBytes(uint32_t bytes) const { IgnoreBits(bytes2bits(bytes)); }

  void Devance(uint32_t bits) { bitsUsed -= bits; }

  void SetWriteOffset(uint32_t offs) { bitsUsed = offs; }

  void SetUnalignedWriteOffset(uint32_t offs) { bitsUsed = offs; }

  void AlignWriteToByteBoundary() {
    if (bitsUsed)
      bitsUsed += 8 - (((bitsUsed - 1) & 7) + 1);
  }

  void AlignReadToByteBoundary() const {
    if (readOffset)
      readOffset += 8 - (((readOffset - 1) & 7) + 1);
  }

  void ResetWritePointer() { bitsUsed = 0; }

  void ResetReadPointer() const { readOffset = 0; }


  // actual read/write implementation
  void WriteBits(const uint8_t *input, uint32_t bits) {
    if (!input || !bits)
      return;
    reserveBits(bits);

    uint32_t bitsUsedMod8 = bitsUsed & 7;
    uint32_t bitsMod8 = bits & 7;
    const uint8_t *srcPtr = input;
    uint8_t *destPtr = GetData() + (bitsUsed >> 3);

    if (!bitsUsedMod8 && !bitsMod8) {
      memcpy(destPtr, srcPtr, bits >> 3);
      bitsUsed += bits;
      return;
    }

    uint8_t upShift = 8 - bitsUsedMod8; // also how many remaining free bits in byte left
    uint8_t srcByte;
    uint8_t destByte = *destPtr & (0xff << upShift); // clear low bits

    bitsUsed += bits;

    for (; bits >= 8; bits -= 8) {
      srcByte = *srcPtr++;
      *destPtr++ = destByte | (srcByte >> bitsUsedMod8);
      destByte = srcByte << upShift;
    }
    if (!bits) {
      *destPtr = destByte | (*destPtr & (0xff >> bitsUsedMod8));
      return;
    }
    srcByte = *srcPtr & ((1 << bits) - 1);
    int bitsDiff = (int) bits - (int) upShift;
    if (bitsDiff <= 0) // enough space left in byte to fit remaining bits?
    {
      *destPtr = destByte | (*destPtr & (0xff >> (bits + bitsUsedMod8))) | (srcByte << -bitsDiff);
      return;
    }
    *destPtr++ = destByte | (srcByte >> bitsDiff);
    bits -= upShift;
    *destPtr = (*destPtr & (0xff >> bits)) | ((srcByte & ((1 << bits) - 1)) << (8 - bits));
  }

  bool ReadBits(uint8_t *output, uint32_t bits) const {
    if (!bits)
      return true;
    else if ((readOffset + bits) > bitsUsed) {
      return false;
    }

    const uint8_t *dataPtr = GetData();
    uint32_t readmod8 = readOffset & 7;
    if (readmod8 == 0 && (bits & 7) == 0) // fast path - everything byte aligned
    {
      memcpy(output, dataPtr + bits2bytes(readOffset), bits2bytes(bits));
      readOffset += bits;
      return true;
    }

    memset(output, 0, bits2bytes(bits));

    uint32_t offs = 0;
    while (bits > 0) {
      *(output + offs) |= *(dataPtr + (readOffset >> 3)) << readmod8;

      if (readmod8 > 0 && bits > (8 - readmod8))
        *(output + offs) |= *(dataPtr + (readOffset >> 3) + 1) >> (8 - readmod8);

      if (bits >= 8) {
        bits -= 8;
        readOffset += 8;
        offs++;
      } else {
        *(output + offs) >>= 8 - bits;
        readOffset += bits;
        break;
      }
    }
    return true;
  }

  void Write(const char *input, uint32_t lenInBytes) { WriteBits((const uint8_t *) input, bytes2bits(lenInBytes)); }

  bool Read(char *output, uint32_t lenInBytes) const { return ReadBits((uint8_t *) output, bytes2bits(lenInBytes)); }

  template<typename T>
  void Write(const T &t) {
    WriteBits((const uint8_t *) &t, bytes2bits(sizeof(T)));
  }

  // bools
  void Write0()
  {
    reserveBits(1);
    const uint32_t bitsmod8 = bitsUsed & 7;
    if (!(bitsUsed & 7))
      GetData()[bitsUsed >> 3] = 0;
    else
      GetData()[bitsUsed >> 3] &= ~(0x80 >> bitsmod8);
    bitsUsed++;
  }
  void Write1()
  {
    reserveBits(1);
    const uint32_t bitsmod8 = bitsUsed & 7;
    if (!bitsmod8)
      GetData()[bitsUsed >> 3] = 0x80;
    else
      GetData()[bitsUsed >> 3] |= 0x80 >> bitsmod8;
    bitsUsed++;
  }

  void Write(bool f)
  {
    f ? Write1() : Write0();
  }

  void Write(const std::string &str)
  {
    writeString(str.c_str(), str.size());
  }

  template<typename T>
  bool Read(T &t) const {
    return ReadBits((uint8_t *) &t, bytes2bits(sizeof(T)));
  }

  bool ReadBit() const {
    bool r = (GetData()[readOffset >> 3] & (0x80 >> (readOffset & 7))) != 0;
    readOffset++;
    return r;
  }

  bool Read(bool &v) const {
    if (readOffset + 1 > bitsUsed)
      return false;
    v = ReadBit();
    return true;
  }

  template<typename T>
  void WriteAt(T t, uint32_t pos) {
    const uint32_t curPos = GetWriteOffset();
    SetWriteOffset(pos);
    Write(t);
    SetWriteOffset(curPos);
  }


  template <typename T>
  void WriteArray(T *tptr, size_t cnt)
  {
    WriteBits((const uint8_t *)tptr, (uint32_t)bytes2bits(sizeof(T) * cnt));
  }
  template <typename T>
  inline bool ReadArray(T *tptr, size_t cnt) const
  {
    return ReadBits((uint8_t *)tptr, (uint32_t)bytes2bits(sizeof(T) * cnt));
  }


  bool Read(std::string &str) const {
    uint32_t sz;
    if (!ReadCompressed(sz))
      return false;
    str.resize(sz);
    if (!Read(str.data(), sz))
      return false;
    return true;
  }


  void WriteCompressed(int v) { writeCompressedUnsignedGeneric(v); }

  bool ReadCompressed(int &v) const { return readCompressedUnsignedGeneric(v); }

  void WriteCompressed(uint64_t v) { writeCompressedUnsignedGeneric(v); }

  void WriteCompressed(uint32_t v) { writeCompressedUnsignedGeneric(v); }

  bool ReadCompressed(uint64_t &v) const { return readCompressedUnsignedGeneric(v); }

  bool ReadCompressed(uint32_t &v) const { return readCompressedUnsignedGeneric(v); }

  void WriteCompressed(uint16_t v) { writeCompressedUnsignedGeneric(v); }

  bool ReadCompressed(uint16_t &v) const { return readCompressedUnsignedGeneric(v); }

  void Write(const char *t) { writeString(t, (t && *t) ? strlen(t) : size_t(0)); }

  void Write(char *t) { writeString(t, (t && *t) ? strlen(t) : size_t(0)); }

  void reserveBits(uint32_t bits) {
    if (bits == 0)
      return;

    uint32_t newBitsAllocated = bitsUsed + bits;
    if (!bitsAllocated || bitsAllocated < newBitsAllocated) {
      size_t newBytesAllocated = std::max(bitsAllocated ? (bitsAllocated >> 3) * 2u : 16u,
                                          bits2bytes(newBitsAllocated));
      if (!data)
        data = (uint8_t *) malloc(newBytesAllocated);
      else
        data = (uint8_t *) realloc(data, newBytesAllocated);

      if (size_t tailBytes = newBytesAllocated - (bitsAllocated >> 3))
        memset(data + (bitsAllocated >> 3), 0, tailBytes);
      dataOwner = 1;
      bitsAllocated = (uint32_t) bytes2bits(newBytesAllocated);
    }
  }

protected:
  template<typename T>
  void writeCompressedUnsignedGeneric(T v) {
    for (int i = 0; i < (int) sizeof(v) + 1; ++i) {
      uint8_t byte = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
      Write(byte);
      v >>= 7;
      if (!v)
        break;
    }
  }

  template<typename T>
  bool readCompressedUnsignedGeneric(T &v) const {
    v = 0;
    for (int i = 0; i < (int) sizeof(v) + 1; ++i) {
      uint8_t byte = 0;
      if (!Read(byte))
        return false;
      v |= T(byte & ~(1 << 7)) << (i * 7);
      if ((byte & (1 << 7)) == 0)
        break;
    }
    return true;
  }

  void writeString(const char *str, size_t str_len) {
    WriteCompressed((uint32_t) str_len);
    if (str_len)
      Write(str, (uint32_t) str_len);
  }

  void DuplicateBitStream(const BitStream &bs) {
    bitsUsed = bs.bitsUsed;
    dataOwner = 1;
    readOffset = bs.readOffset;
    data = bs.bitsAllocated ? (uint8_t *) malloc(bits2bytes(bs.bitsAllocated)) : nullptr;
    memcpy(data, bs.data, bits2bytes(bs.bitsAllocated));
    bitsAllocated = (uint32_t) bytes2bits(bits2bytes(bs.bitsAllocated));
  }

  static inline uint32_t bits2bytes(uint32_t bi) { return (bi + 7) >> 3; }

  static inline uint32_t bytes2bits(uint32_t by) { return by << 3; }

  uint32_t bitsUsed: 31;
  uint32_t dataOwner: 1;
  uint32_t bitsAllocated;
  mutable uint32_t readOffset;
  uint8_t *data;

};

#endif