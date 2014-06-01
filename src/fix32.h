#ifndef IISPACE_SRC_FIX32_H
#define IISPACE_SRC_FIX32_H

#include <cstdint>
#include <iomanip>
#include <ostream>
#include <sstream>

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

class fixed {
public:

  inline fixed();
  inline fixed(const fixed& f);
  inline fixed(int32_t v);

  inline static fixed from_internal(int64_t f);
  inline int64_t to_internal() const;
  inline int32_t to_int() const;
  inline float to_float() const;
  inline explicit operator bool() const;

  // Assignment operators.
  inline fixed& operator=(const fixed& f);
  inline fixed& operator+=(const fixed& f);
  inline fixed& operator-=(const fixed& f);
  inline fixed& operator*=(const fixed& f);
  inline fixed& operator/=(const fixed& f);

  // Arithmetic operators.
  inline fixed operator+(const fixed& f) const;
  inline fixed operator-(const fixed& f) const;
  inline fixed operator*(const fixed& f) const;
  inline fixed operator/(const fixed& f) const;
  inline fixed operator-() const;

  // Comparison operators.
  inline bool operator==(const fixed& f) const;
  inline bool operator!=(const fixed& f) const;
  inline bool operator<=(const fixed& f) const;
  inline bool operator>=(const fixed& f) const;
  inline bool operator<(const fixed& f) const;
  inline bool operator>(const fixed& f) const;

  // Math functions.
  inline fixed abs() const;
  inline fixed sqrt() const;
  inline fixed sin() const;
  inline fixed cos() const;
  inline fixed atan2(const fixed& x) const;

  // Constants.
  static const fixed pi;
  static const fixed tenth;
  static const fixed hundredth;
  static const fixed half;
  static const fixed quarter;
  static const fixed eighth;
  static const fixed sixteenth;

private:

  friend std::ostream& operator<<(std::ostream& o, const fixed& f);
  int64_t _value;

};

inline std::ostream& operator<<(std::ostream& o, const fixed& f)
{
  if (f._value < 0) {
    o << "-";
  }

  uint64_t v = f.abs()._value;
  o << (v >> 32) << ".";
  double d = 0;
  double val = 0.5;
  for (std::size_t i = 0; i < 32; ++i) {
    if (v & (int64_t(1) << (31 - i))) {
      d += val;
    }
    val /= 2;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(16) << d;
  o << ss.str().substr(2);
  return o;
}

inline fixed operator+(int32_t v, const fixed& f)
{
  return fixed(v) + f;
}

inline fixed operator-(int32_t v, const fixed& f)
{
  return fixed(v) - f;
}

inline fixed operator*(int32_t v, const fixed& f)
{
  return fixed(v) * f;
}

inline fixed operator/(int32_t v, const fixed& f)
{
  return fixed(v) / f;
}

inline bool operator==(int32_t v, const fixed& f)
{
  return fixed(v) == f;
}

inline bool operator!=(int32_t v, const fixed& f)
{
  return fixed(v) != f;
}

inline bool operator<=(int32_t v, const fixed& f)
{
  return fixed(v) <= f;
}

inline bool operator>=(int32_t v, const fixed& f)
{
  return fixed(v) >= f;
}

inline bool operator<(int32_t v, const fixed& f)
{
  return fixed(v) < f;
}

inline bool operator>(int32_t v, const fixed& f)
{
  return fixed(v) > f;
}

fixed::fixed()
  : _value(0)
{
}

fixed::fixed(const fixed& f)
  : _value(f._value)
{
}

fixed::fixed(int32_t v)
  : _value(int64_t(v) << 32)
{
}

fixed fixed::from_internal(int64_t f)
{
  fixed r;
  r._value = f;
  return r;
}

int64_t fixed::to_internal() const
{
  return _value;
}

int32_t fixed::to_int() const
{
  return _value >> 32;
}

float fixed::to_float() const
{
  return float(_value) / (uint64_t(1) << 32);
}

fixed::operator bool() const
{
  return _value != 0;
}

fixed& fixed::operator=(const fixed& f)
{
  _value = f._value;
  return *this;
}

fixed& fixed::operator+=(const fixed& f)
{
  return *this = *this + f;
}

fixed& fixed::operator-=(const fixed& f)
{
  return *this = *this - f;
}

fixed& fixed::operator*=(const fixed& f)
{
  return *this = *this * f;
}

fixed& fixed::operator/=(const fixed& f)
{
  return *this = *this / f;
}

fixed fixed::operator+(const fixed& f) const
{
  return from_internal(_value + f._value);
}

fixed fixed::operator-(const fixed& f) const
{
  return from_internal(_value - f._value);
}

fixed fixed::operator*(const fixed& f) const
{
  int64_t sign = fixed_sgn(_value, f._value);
  uint64_t l = abs()._value;
  uint64_t r = f.abs()._value;

  static const uint64_t mask = 0xffffffff;
  uint64_t hi = (l >> 32) * (r >> 32);
  uint64_t lo = (l & mask) * (r & mask);
  uint64_t lf = (l >> 32) * (r & mask);
  uint64_t rt = (l & mask) * (r >> 32);

  uint64_t combine = (hi << 32) + lf + rt + (lo >> 32);
  return from_internal(sign * combine);
}

fixed fixed::operator/(const fixed& f) const
{
  int64_t sign = fixed_sgn(_value, f._value);
  uint64_t r = abs()._value;
  uint64_t d = f.abs()._value;
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
    if (shift > bit) {
      shift = bit;
    }
    r <<= shift;
    bit -= shift;

    uint64_t div = r / d;
    r = r % d;
    q += div << bit;

    r <<= 1;
    if (!bit) {
      break;
    }
    bit--;
  }
  return from_internal(sign * (q >> 1));
}

fixed fixed::operator-() const
{
  return fixed() - *this;
}

bool fixed::operator==(const fixed& f) const
{
  return _value == f._value;
}

bool fixed::operator!=(const fixed& f) const
{
  return _value != f._value;
}

bool fixed::operator<=(const fixed& f) const
{
  return _value <= f._value;
}

bool fixed::operator>=(const fixed& f) const
{
  return _value >= f._value;
}

bool fixed::operator<(const fixed& f) const
{
  return _value < f._value;
}

bool fixed::operator>(const fixed& f) const
{
  return _value > f._value;
}

fixed fixed::abs() const
{
  return from_internal((_value ^ (_value >> 63)) - (_value >> 63));
}

fixed fixed::sqrt() const
{
  if (_value <= 0) {
    return 0;
  }
  if (*this < 1) {
    return 1 / (1 / *this).sqrt();
  }

  const fixed a = *this / 2;
  static const fixed half = fixed(1) / 2;
  static const fixed bound = fixed(1) / 1024;

  fixed f = from_internal(_value >> ((32 - clz(_value)) / 2));
  for (std::size_t n = 0; n < 8 && f; ++n) {
    f = f * half + a / f;
    if (from_internal((f * f).abs()._value - _value) < bound) {
      break;
    }
  }
  return f;
}

fixed fixed::sin() const
{
  bool negative = _value < 0;
  fixed angle = from_internal(abs()._value % (2 * pi)._value);

  if (angle > pi) {
    angle -= 2 * pi;
  }

  fixed angle2 = angle * angle;
  fixed out = angle;

  static const fixed r6 = fixed(1) / 6;
  static const fixed r120 = fixed(1) / 120;
  static const fixed r5040 = fixed(1) / 5040;
  static const fixed r362880 = fixed(1) / 362880;
  static const fixed r39916800 = fixed(1) / 39916800;

  angle *= angle2;
  out -= angle * r6;
  angle *= angle2;
  out += angle * r120;
  angle *= angle2;
  out -= angle * r5040;
  angle *= angle2;
  out += angle * r362880;
  angle *= angle2;
  out -= angle * r39916800;

  return negative ? -out : out;
}

fixed fixed::cos() const
{
  return (*this + pi / 2).sin();
}

fixed fixed::atan2(const fixed& x) const
{
  fixed y = abs();
  fixed angle;

  if (x._value >= 0) {
    if (x + y == 0) {
      return -pi / 4;
    }
    fixed r = (x - y) / (x + y);
    fixed r3 = r * r * r;
    angle =
        from_internal(0x32400000) * r3 -
        from_internal(0xfb500000) * r + pi / 4;
  }
  else {
    if (x - y == 0) {
      return -3 * pi / 4;
    }
    fixed r = (x + y) / (y - x);
    fixed r3 = r * r * r;
    angle =
        from_internal(0x32400000) * r3 -
        from_internal(0xfb500000) * r + 3 * pi / 4;
  }
  return _value < 0 ? -angle : angle;
}

#endif
