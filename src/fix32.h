#ifndef FIX32_H
#define FIX32_H

#include <cstdint>
#include <cmath>
#include <ostream>

inline int64_t fixed_abs(int64_t f)
{
  return (f ^ (f >> 63)) - (f >> 63);
}

inline int64_t fixed_sgn(int64_t a, int64_t b)
{
  return (((a < 0) == (b < 0)) << 1) - 1;
}

// MATT BELL CODE
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#ifdef PLATFORM_64
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

inline uint8_t clz(uint64_t x)
{
#ifdef __GNUC__
  if (x == 0)
    return 64;
  return __builtin_clzll(x);
#else
#ifdef _MSC_VER
  unsigned long t = 0;
#ifdef PLATFORM_64
  if (x == 0)
    return 64;
  _BitScanReverse64(&t, x);
  return 63 - uint8_t(t);
#else
  if (x > 0xffffffff) {
    _BitScanReverse(&t, uint32_t(x >> 32));
    return 31 - uint8_t(t);
  }
  if (x == 0) {
    return 64;
  }
  _BitScanReverse(&t, uint32_t(x & 0xffffffff));
  return 63 - uint8_t(t);
#endif
#else
  if (x == 0) {
    return 64;
  }
  uint8_t t = 0;
#ifdef PLATFORM_LINUX
  if (!(x & 0xffff000000000000ULL)) {
    t += 16;
    x <<= 16;
  }
  while (!(x & 0xff00000000000000ULL)) {
    t += 8;
    x <<= 8;
  }
  while (!(x & 0xf000000000000000ULL)) {
    t += 4;
    x <<= 4;
  }
  while (!(x & 0x8000000000000000ULL)) {
    t += 1;
    x <<= 1;
  }
#else
  if (!(x & 0xffff000000000000)) {
    t += 16;
    x <<= 16;
  }
  while (!(x & 0xff00000000000000)) {
    t += 8;
    x <<= 8;
  }
  while (!(x & 0xf000000000000000)) {
    t += 4;
    x <<= 4;
  }
  while (!(x & 0x8000000000000000)) {
    t += 1;
    x <<= 1;
  }
#endif
  return t;
#endif
#endif
}

class Fix32 {
public:

  int64_t _value;

  Fix32()
    : _value(0) {}

  Fix32(const Fix32& f)
  {
    _value = f._value;
  }

  static Fix32 from_internal(int64_t f)
  {
    Fix32 r;
    r._value = f;
    return r;
  }

  Fix32(int32_t v)
  {
    _value = int64_t(v) << 32;
  }

  inline int32_t to_int() const
  {
    return _value >> 32;
  }

  inline float to_float() const
  {
    return float(_value) / (uint64_t(1) << 32);
  }

  inline Fix32 operator-() const
  {
    return Fix32() - *this;
  }

  inline Fix32& operator=(const Fix32& f)
  {
    _value = f._value;
    return *this;
  }

  inline Fix32& operator+=(const Fix32& f)
  {
    _value = (*this + f)._value;
    return *this;
  }

  inline Fix32& operator-=(const Fix32& f)
  {
    _value = (*this - f)._value;
    return *this;
  }

  inline Fix32& operator*=(const Fix32& f)
  {
    _value = (*this * f)._value;
    return *this;
  }

  inline Fix32& operator/=(const Fix32& f)
  {
    _value = (*this / f)._value;
    return *this;
  }

  inline Fix32 operator+(const Fix32& f) const
  {
    return from_internal(_value + f._value);
  }

  inline Fix32 operator-(const Fix32& f) const
  {
    return from_internal(_value - f._value);
  }

  inline Fix32 operator*(const Fix32& f) const
  {
    int64_t sign = fixed_sgn(_value, f._value);
    uint64_t l = fixed_abs(_value);
    uint64_t r = fixed_abs(f._value);
    const uint64_t frc = 0xffffffff;

    uint64_t hi = (l >> 32) * (r >> 32);
    uint64_t lo = (l & frc) * (r & frc);
    uint64_t lf = (l >> 32) * (r & frc);
    uint64_t rt = (l & frc) * (r >> 32);
    uint64_t xy = (hi << 32) + lf + rt + (lo >> 32);
    return from_internal(sign * xy);
  }

  inline Fix32 operator/(const Fix32& f) const
  {
    int64_t sign = fixed_sgn(_value, f._value);
    uint64_t r = fixed_abs(_value);
    uint64_t d = fixed_abs(f._value);
    uint64_t q = 0;

    uint64_t bit = 33;
    while (!(d & 0xf) && bit >= 4) {
      d >>= 4;
      bit -= 4;
    }
    if (d == 0) {
      return 0;
    }
    while (r) {
      uint64_t shift = clz(r);
      if (shift > bit)
        shift = bit;
      r <<= shift;
      bit -= shift;

      uint64_t div = r / d;
      r = r % d;
      q += div << bit;

      r <<= 1;
      if (!bit)
        break;
      bit--;
    }
    return from_internal(sign * (q >> 1));
  }

  inline bool operator==(const Fix32& f) const
  {
    return _value == f._value;
  }

  inline bool operator!=(const Fix32& f) const
  {
    return _value != f._value;
  }

  inline bool operator<=(const Fix32& f) const
  {
    return _value <= f._value;
  }

  inline bool operator>=(const Fix32& f) const
  {
    return _value >= f._value;
  }

  inline bool operator<(const Fix32& f) const
  {
    return _value < f._value;
  }

  inline bool operator>(const Fix32& f) const
  {
    return _value > f._value;
  }

  Fix32 sqrt() const;
  Fix32 sin() const;
  Fix32 cos() const;
  Fix32 atan2(const Fix32& x) const;

};

#ifdef PLATFORM_LINUX
const Fix32 F32PI = Fix32::from_internal(0x3243f6a88ULL);
#else
const Fix32 F32PI = Fix32::from_internal(0x3243f6a88);
#endif
const Fix32 F32ONE = Fix32(1);

inline Fix32 operator+(int v, const Fix32& f)
{
  return Fix32(v) + f;
}

inline Fix32 operator-(int v, const Fix32& f)
{
  return Fix32(v) - f;
}

inline Fix32 operator*(int v, const Fix32& f)
{
  return Fix32(v) * f;
}

inline Fix32 operator/(int v, const Fix32& f)
{
  return Fix32(v) / f;
}

inline bool operator==(int v, const Fix32& f)
{
  return Fix32(v) == f;
}

inline bool operator!=(int v, const Fix32& f)
{
  return Fix32(v) != f;
}

inline bool operator<=(int v, const Fix32& f)
{
  return Fix32(v) <= f;
}

inline bool operator>=(int v, const Fix32& f)
{
  return Fix32(v) >= f;
}

inline bool operator<(int v, const Fix32& f)
{
  return Fix32(v) < f;
}

inline bool operator>(int v, const Fix32& f)
{
  return Fix32(v) > f;
}

std::ostream& operator<<(std::ostream& o, const Fix32& f);

#endif
