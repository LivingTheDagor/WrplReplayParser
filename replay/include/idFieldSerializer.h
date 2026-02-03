//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

#pragma once


#include <cstdint>
#include "BitStream.h"
#include "consts.h"
#include <array>

#ifndef USHRT_MAX
#define USHRT_MAX 65535
#endif
///@class IdFieldSerializer32
///@brief this is a workhorse for minimal serialization backwards compatibility through persistent ids
///@note intended to be a base class for MPI
///@note call order for writing is <Write Placeholders> <Write Body> <writeFieldsSize()>
///      and reverse for reading <readFieldsSizeAndFlag()> <Read Body>
///
class IdFieldSerializer32
{
  static const uint8_t MAX_FIELDS_NUM = 32;

public:
  IdFieldSerializer32();
  IdFieldSerializer32(const IdFieldSerializer32 &other);
  IdFieldSerializer32 &operator=(const IdFieldSerializer32 &other);

  void setFieldSize(uint32_t sz);

  void checkFieldSize(uint8_t index, BitSize_t sz) const;

  void writeFieldsSize(BitStream &to) const;
  uint32_t readFieldsSizeAndFlag(const BitStream &from);

  void skipReadingField(uint8_t index, const BitStream &from) const;

private:
  std::array<uint32_t, MAX_FIELDS_NUM> sizes;
  uint8_t currWrSz;
  uint8_t currRdSz;
};

///@class IdFieldSerializer255
///@brief this is a workhorse for minimal serialization backwards compatibility through persistent ids
///@note NOT intended for direct use
///@note call order for writing is <Write Placeholders> <Write Body> [writeFieldsIndex()] <writeFieldsSize()>
///      and reverse for reading <readFieldsSizeAndCount()> [readFieldsIndex()] <Read Body>
///      [] brackets mean optional
///
class IdFieldSerializer255
{
  static const int BITS_PER_COUNT = 12;
  static const int BIT_MASK_COUNT = (1 << BITS_PER_COUNT) - 1;

public:
  typedef uint16_t Id;
  typedef Id Index;
  static const Index MAX_FIELDS_NUM = 255;
    static_assert(MAX_FIELDS_NUM < (1 << BITS_PER_COUNT)); // 12 bits for count and 4 for size of id in bits

  IdFieldSerializer255();
  IdFieldSerializer255(const IdFieldSerializer255 &other) = delete;
  IdFieldSerializer255 &operator=(const IdFieldSerializer255 &other) = delete;

  void reset();

  void setFieldId(Id id);
  void setFieldSize(uint32_t sz);

  [[nodiscard]] Id getFieldId(Index index) const;
  [[nodiscard]] uint32_t getFieldSize(Index index) const;

  void writeFieldsSize(BitStream &to, BitSize_t at = 0) const;
  Index readFieldsSizeAndCount(const BitStream &from, BitSize_t &end);

  void skipReadingField(Index index, const BitStream &from) const;

  void writeFieldsIndex(BitStream &to, BitSize_t at = 0) const;
  bool readFieldsIndex(const BitStream &from);

private:
  std::array<uint32_t, MAX_FIELDS_NUM> sizes;
  std::array<Id, MAX_FIELDS_NUM> indices;
  Index currWrId;
  Index currWrSz;
  Index currRdSz;
  Index bitsPerId;
};
