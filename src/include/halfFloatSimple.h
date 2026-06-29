//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <dag_assert.h>

/// simple half float implementation with custom mantissa/exponent/bias
/// without mathematics, only for storing/serialization

const int min_exponent_size = 3;
const int max_exponent_size = 8;

float half_float_ex_decode(uint16_t v, bool positive, int exponent_size, int bias);
uint16_t half_float_ex_encode(float v, bool positive, int exponent_size, int bias);

template<int EXPONENT_SIZE, int EXPONENT_BIAS>
struct HalfFloatSimpleEx {
  uint16_t val;

  HalfFloatSimpleEx() = default;

  HalfFloatSimpleEx(const HalfFloatSimpleEx &other) : val(other.val) {}

  HalfFloatSimpleEx(float v) {
    G_STATIC_ASSERT(EXPONENT_SIZE >= min_exponent_size && EXPONENT_SIZE <= max_exponent_size);
    G_STATIC_ASSERT(EXPONENT_BIAS >= 1 && EXPONENT_BIAS < (1 << EXPONENT_SIZE));
    val = half_float_ex_encode(v, false, EXPONENT_SIZE, EXPONENT_BIAS);
  }

  HalfFloatSimpleEx &operator=(const HalfFloatSimpleEx &other) = default;

  HalfFloatSimpleEx &operator=(float v) {
    val = half_float_ex_encode(v, false, EXPONENT_SIZE, EXPONENT_BIAS);
    return *this;
  }

  operator float() const { return half_float_ex_decode(val, false, EXPONENT_SIZE, EXPONENT_BIAS); }

  bool operator==(const HalfFloatSimpleEx &other) const { return val == other.val; }
};

template<int EXPONENT_SIZE, int EXPONENT_BIAS>
struct HalfFloatSimplePositiveEx {
  uint16_t val;

  HalfFloatSimplePositiveEx() = default;

  HalfFloatSimplePositiveEx(const HalfFloatSimplePositiveEx &other) : val(other.val) {}

  HalfFloatSimplePositiveEx(float v) {
    G_STATIC_ASSERT(EXPONENT_SIZE >= min_exponent_size && EXPONENT_SIZE <= max_exponent_size);
    G_STATIC_ASSERT(EXPONENT_BIAS >= 0 && EXPONENT_BIAS < (1 << EXPONENT_SIZE));
    val = half_float_ex_encode(v, true, EXPONENT_SIZE, EXPONENT_BIAS);
  }

  HalfFloatSimplePositiveEx &operator=(const HalfFloatSimplePositiveEx &other) = default;

  HalfFloatSimplePositiveEx &operator=(float v) {
    val = half_float_ex_encode(v, true, EXPONENT_SIZE, EXPONENT_BIAS);
    return *this;
  }

  operator float() const { return half_float_ex_decode(val, true, EXPONENT_SIZE, EXPONENT_BIAS); }

  bool operator==(const HalfFloatSimplePositiveEx &other) const { return val == other.val; }
};

using HalfFloatSimplePositive = HalfFloatSimplePositiveEx<5, 15>;


const uint32_t signMask = 0x80000000U;
const uint32_t exponentMask = 0x7F800000U;
const uint32_t fractionMask = 0x7FFFFFU;
const int float_fraction_size = 23;
const int float_bias = 127;

inline uint16_t calc_fraction_size(bool positive, int exponent_size) { return 16 - exponent_size - (positive ? 0 : 1); }

inline int calc_max_exp(int exponent_size) { return (1 << exponent_size) - 1; }

float half_float_ex_decode(uint16_t v, bool positive, int exponent_size, int bias) {
  int fractionSize = calc_fraction_size(positive, exponent_size);
  int maxExp = calc_max_exp(exponent_size);

  int exponent = int((positive ? v : (v & 0x7FFFU)) >> fractionSize);
  bool isNan = exponent >= maxExp;
  uint32_t exponentPart =
    exponent <= 0 ? 0U : (isNan ? exponentMask : ((exponent - bias + float_bias) << float_fraction_size));
  uint32_t signPart = !positive && (v & 0x8000U) ? signMask : 0;
  uint32_t fractionPart = isNan ? fractionMask : (uint32_t(v) << (float_fraction_size - fractionSize)) & fractionMask;

  union {
    float f;
    uint32_t i;
  } flt;
  flt.i = signPart + exponentPart + fractionPart;

  return flt.f;
}

uint16_t half_float_ex_encode(float v, bool positive, int exponent_size, int bias) {
  G_ASSERT_AND_DO(!positive || v >= 0.f, v = 0.f);

  int fractionSize = calc_fraction_size(positive, exponent_size);
  int maxExp = calc_max_exp(exponent_size);

  union {
    float f;
    uint32_t i;
  } flt;
  flt.f = v;

  int exponent = ((flt.i & exponentMask) >> float_fraction_size) - float_bias + bias;
  uint32_t exponentPart = clamp(exponent, 0, maxExp) << fractionSize;
  uint32_t signPart = positive ? 0 : ((flt.i & signMask) >> 16);
  uint32_t fractionPart = exponent >= 0 ? ((flt.i & fractionMask) >> (float_fraction_size - fractionSize)) : 0U;

  return signPart + exponentPart + fractionPart;
}