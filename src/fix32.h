#ifndef IISPACE_SRC_FIX32_H
#define IISPACE_SRC_FIX32_H
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <sstream>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#ifdef PLATFORM_64
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

namespace detail {
inline constexpr int64_t fixed_sgn(int64_t a, int64_t b) {
  return (((a < 0) == (b < 0)) << 1) - 1;
}

inline constexpr uint8_t clz(uint64_t x) {
#ifdef __GNUC__
  if (x == 0) {
    return 64;
  }
  return __builtin_clzll(x);
#else
#ifdef _MSC_VER
  unsigned long t = 0;
#ifdef PLATFORM_64
  if (x == 0) {
    return 64;
  }
  _BitScanReverse64(&t, x);
  return 63 - static_cast<uint8_t>(t);
#else
  if (x > 0xffffffff) {
    _BitScanReverse(&t, static_cast<uint32_t>(x >> 32));
    return 31 - static_cast<uint8_t>(t);
  }
  if (x == 0) {
    return 64;
  }
  _BitScanReverse(&t, static_cast<uint32_t>(x & 0xffffffff));
  return 63 - static_cast<uint8_t>(t);
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
}

class fixed {
public:
  constexpr fixed() : _value{0} {}
  constexpr fixed(const fixed& f) : _value{f._value} {}
  constexpr fixed(int32_t v) : _value{static_cast<int64_t>(v) << 32} {}

  static constexpr fixed from_internal(int64_t v) {
    fixed f;
    f._value = v;
    return f;
  }

  constexpr int64_t to_internal() const {
    return _value;
  }

  constexpr int32_t to_int() const {
    return _value >> 32;
  }

  constexpr float to_float() const {
    return static_cast<float>(_value) / (uint64_t{1} << 32);
  }

  constexpr explicit operator bool() const {
    return _value != 0;
  }

  constexpr fixed& operator=(const fixed& f) {
    _value = f._value;
    return *this;
  }

  constexpr fixed& operator+=(const fixed& f) {
    return *this = *this + f;
  }

  constexpr fixed& operator-=(const fixed& f) {
    return *this = *this - f;
  }

  constexpr fixed& operator*=(const fixed& f) {
    return *this = *this * f;
  }

  constexpr fixed& operator/=(const fixed& f) {
    return *this = *this / f;
  }

  constexpr fixed abs() const;
  constexpr fixed sqrt() const;
  constexpr fixed sin() const;
  constexpr fixed cos() const;
  constexpr fixed atan2(const fixed& x) const;

private:
  friend constexpr bool operator==(const fixed&, const fixed&);
  friend constexpr bool operator!=(const fixed&, const fixed&);
  friend constexpr bool operator<=(const fixed&, const fixed&);
  friend constexpr bool operator>=(const fixed&, const fixed&);
  friend constexpr bool operator<(const fixed&, const fixed&);
  friend constexpr bool operator>(const fixed&, const fixed&);
  friend constexpr fixed operator<<(const fixed&, int32_t);
  friend constexpr fixed operator>>(const fixed&, int32_t);
  friend constexpr fixed operator-(const fixed&);
  friend constexpr fixed operator+(const fixed&, const fixed&);
  friend constexpr fixed operator-(const fixed&, const fixed&);
  friend constexpr fixed operator*(const fixed&, const fixed&);
  friend constexpr fixed operator/(const fixed&, const fixed&);
  friend std::ostream& operator<<(std::ostream&, const fixed&);
  int64_t _value;
};

inline constexpr bool operator==(const fixed& a, const fixed& b) {
  return a._value == b._value;
}

inline constexpr bool operator!=(const fixed& a, const fixed& b) {
  return a._value != b._value;
}

inline constexpr bool operator<=(const fixed& a, const fixed& b) {
  return a._value <= b._value;
}

inline constexpr bool operator>=(const fixed& a, const fixed& b) {
  return a._value >= b._value;
}

inline constexpr bool operator<(const fixed& a, const fixed& b) {
  return a._value < b._value;
}

inline constexpr bool operator>(const fixed& a, const fixed& b) {
  return a._value > b._value;
}

inline constexpr fixed operator<<(const fixed& a, int32_t b) {
  return fixed::from_internal(a._value << b);
}

inline constexpr fixed operator>>(const fixed& a, int32_t b) {
  return fixed::from_internal(a._value >> b);
}

inline constexpr fixed operator-(const fixed& f) {
  return 0 - f;
}

inline constexpr fixed operator+(const fixed& a, const fixed& b) {
  return fixed::from_internal(a._value + b._value);
}

inline constexpr fixed operator-(const fixed& a, const fixed& b) {
  return fixed::from_internal(a._value - b._value);
}

inline constexpr fixed fixed::abs() const {
  return from_internal((_value ^ (_value >> 63)) - (_value >> 63));
}

inline constexpr fixed operator*(const fixed& a, const fixed& b) {
  int64_t sign = detail::fixed_sgn(a._value, b._value);
  uint64_t l = a.abs()._value;
  uint64_t r = b.abs()._value;

  constexpr uint64_t mask = 0xffffffff;
  uint64_t hi = (l >> 32) * (r >> 32);
  uint64_t lo = (l & mask) * (r & mask);
  uint64_t lf = (l >> 32) * (r & mask);
  uint64_t rt = (l & mask) * (r >> 32);

  uint64_t combine = (hi << 32) + lf + rt + (lo >> 32);
  return fixed::from_internal(sign * combine);
}

inline constexpr fixed operator/(const fixed& a, const fixed& b) {
  int64_t sign = detail::fixed_sgn(a._value, b._value);
  uint64_t r = a.abs()._value;
  uint64_t d = b.abs()._value;
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
    uint64_t shift = detail::clz(r);
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
    --bit;
  }
  return fixed::from_internal(sign * (q >> 1));
}

inline std::ostream& operator<<(std::ostream& o, const fixed& f) {
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

namespace fixed_c {
constexpr fixed pi = fixed::from_internal(0x3243f6a88);
constexpr fixed tenth = fixed{1} / 10;
constexpr fixed hundredth = fixed{1} / 100;
constexpr fixed half = fixed{1} >> 1;
constexpr fixed quarter = fixed{1} >> 2;
constexpr fixed eighth = fixed{1} >> 4;
constexpr fixed sixteenth = fixed{1} >> 8;
}

inline constexpr fixed fixed::sqrt() const {
  if (_value <= 0) {
    return 0;
  }
  if (*this < 1) {
    return 1 / (1 / *this).sqrt();
  }

  const fixed a = *this / 2;
  constexpr fixed half = fixed{1} >> 1;
  constexpr fixed bound = fixed{1} >> 10;

  auto f = from_internal(_value >> ((32 - detail::clz(_value)) / 2));
  for (std::size_t n = 0; f && n < 8; ++n) {
    f = f * half + a / f;
    if (from_internal((f * f).abs()._value - _value) < bound) {
      break;
    }
  }
  return f;
}

inline constexpr fixed fixed::sin() const {
  auto angle = from_internal(abs()._value % (2 * fixed_c::pi)._value);

  if (angle > fixed_c::pi) {
    angle -= 2 * fixed_c::pi;
  }

  fixed angle2 = angle * angle;
  fixed out = angle;

  constexpr fixed r6 = fixed{1} / 6;
  constexpr fixed r120 = fixed{1} / 120;
  constexpr fixed r5040 = fixed{1} / 5040;
  constexpr fixed r362880 = fixed{1} / 362880;
  constexpr fixed r39916800 = fixed{1} / 39916800;

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

  return _value < 0 ? -out : out;
}

inline constexpr fixed fixed::cos() const {
  return (*this + fixed_c::pi / 2).sin();
}

inline constexpr fixed fixed::atan2(const fixed& x) const {
  auto y = abs();
  fixed angle;

  if (x._value >= 0) {
    if (x + y == 0) {
      return -fixed_c::pi / 4;
    }
    fixed r = (x - y) / (x + y);
    fixed r3 = r * r * r;
    angle = from_internal(0x32400000) * r3 - from_internal(0xfb500000) * r + fixed_c::pi / 4;
  } else {
    if (x - y == 0) {
      return -3 * fixed_c::pi / 4;
    }
    fixed r = (x + y) / (y - x);
    fixed r3 = r * r * r;
    angle = from_internal(0x32400000) * r3 - from_internal(0xfb500000) * r + 3 * fixed_c::pi / 4;
  }
  return _value < 0 ? -angle : angle;
}

#endif
