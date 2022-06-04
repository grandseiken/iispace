#ifndef II_GAME_COMMON_FIX32_H
#define II_GAME_COMMON_FIX32_H
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <sstream>

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse64)
#endif

namespace detail {
inline constexpr std::int64_t fixed_sgn(std::int64_t a, std::int64_t b) {
  return (((a < 0) == (b < 0)) << 1) - 1;
}

inline constexpr std::int64_t fixed_abs(std::int64_t a) {
  return (a ^ (a >> 63)) - (a >> 63);
}

inline constexpr uint8_t clz_constexpr(std::int64_t x) {
  if (x == 0) {
    return 64;
  }
  uint8_t t = 0;
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
  return t;
}

inline uint8_t clz(std::uint64_t x) {
#ifdef __GNUC__
  if (x == 0) {
    return 64;
  }
  return __builtin_clzll(x);
#elif _MSC_VER
  unsigned long t = 0;
  if (x == 0) {
    return 64;
  }
  _BitScanReverse64(&t, x);
  return 63 - static_cast<uint8_t>(t);
#else
  return clz_constexpr(x);
#endif
}
}  // namespace detail

class fixed {
public:
  explicit fixed() = default;
  constexpr fixed(std::int32_t v) : value_{static_cast<std::int64_t>(v) << 32} {}
  constexpr fixed(std::uint32_t v) : value_{static_cast<std::int64_t>(v) << 32} {}

  static constexpr fixed from_internal(std::int64_t v) {
    fixed f;
    f.value_ = v;
    return f;
  }

  constexpr std::int64_t to_internal() const {
    return value_;
  }

  constexpr std::int32_t to_int() const {
    return value_ >> 32;
  }

  constexpr float to_float() const {
    constexpr float multiplier = 1.f / (std::uint64_t{1} << 32u);
    return static_cast<float>(value_) * multiplier;
  }

  constexpr explicit operator bool() const {
    return value_ != 0;
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

  fixed& operator/=(const fixed& f) {
    return *this = *this / f;
  }

private:
  friend constexpr bool operator==(const fixed&, const fixed&);
  friend constexpr bool operator!=(const fixed&, const fixed&);
  friend constexpr bool operator<=(const fixed&, const fixed&);
  friend constexpr bool operator>=(const fixed&, const fixed&);
  friend constexpr bool operator<(const fixed&, const fixed&);
  friend constexpr bool operator>(const fixed&, const fixed&);
  friend constexpr fixed operator<<(const fixed&, std::int32_t);
  friend constexpr fixed operator>>(const fixed&, std::int32_t);
  friend constexpr fixed operator-(const fixed&);
  friend constexpr fixed operator+(const fixed&, const fixed&);
  friend constexpr fixed operator-(const fixed&, const fixed&);
  friend constexpr fixed operator*(const fixed&, const fixed&);
  friend fixed operator/(const fixed&, const fixed&);
  friend constexpr fixed constexpr_div(const fixed&, const fixed&);
  friend constexpr fixed abs(const fixed&);
  friend fixed sqrt(const fixed&);
  friend constexpr fixed sin(const fixed&);
  friend constexpr fixed cos(const fixed&);
  friend constexpr fixed atan2(const fixed&, const fixed&);
  friend std::ostream& operator<<(std::ostream&, const fixed&);
  std::int64_t value_;
};

namespace std {
template <>
class numeric_limits<::fixed> {
public:
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool round_style = round_toward_zero;
  static constexpr bool is_iec559 = true;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
};
}  // namespace std

inline constexpr fixed operator"" _fx(unsigned long long v) {
  return fixed{static_cast<std::int32_t>(v)};
}

inline constexpr bool operator==(const fixed& a, const fixed& b) {
  return a.value_ == b.value_;
}

inline constexpr bool operator!=(const fixed& a, const fixed& b) {
  return a.value_ != b.value_;
}

inline constexpr bool operator<=(const fixed& a, const fixed& b) {
  return a.value_ <= b.value_;
}

inline constexpr bool operator>=(const fixed& a, const fixed& b) {
  return a.value_ >= b.value_;
}

inline constexpr bool operator<(const fixed& a, const fixed& b) {
  return a.value_ < b.value_;
}

inline constexpr bool operator>(const fixed& a, const fixed& b) {
  return a.value_ > b.value_;
}

inline constexpr fixed operator<<(const fixed& a, std::int32_t b) {
  return fixed::from_internal(a.value_ << b);
}

inline constexpr fixed operator>>(const fixed& a, std::int32_t b) {
  return fixed::from_internal(a.value_ >> b);
}

inline constexpr fixed operator-(const fixed& f) {
  return 0 - f;
}

inline constexpr fixed operator+(const fixed& a, const fixed& b) {
  return fixed::from_internal(a.value_ + b.value_);
}

inline constexpr fixed operator-(const fixed& a, const fixed& b) {
  return fixed::from_internal(a.value_ - b.value_);
}

inline constexpr fixed operator*(const fixed& a, const fixed& b) {
  std::int64_t sign = detail::fixed_sgn(a.value_, b.value_);
  std::uint64_t l = detail::fixed_abs(a.value_);
  std::uint64_t r = detail::fixed_abs(b.value_);

  constexpr std::uint64_t mask = 0xffffffff;
  std::uint64_t hi = (l >> 32) * (r >> 32);
  std::uint64_t lo = (l & mask) * (r & mask);
  std::uint64_t lf = (l >> 32) * (r & mask);
  std::uint64_t rt = (l & mask) * (r >> 32);

  std::uint64_t combine = (hi << 32) + lf + rt + (lo >> 32);
  return fixed::from_internal(sign * combine);
}

inline fixed operator/(const fixed& a, const fixed& b) {
  std::int64_t sign = detail::fixed_sgn(a.value_, b.value_);
  std::uint64_t r = detail::fixed_abs(a.value_);
  std::uint64_t d = detail::fixed_abs(b.value_);
  std::uint64_t q = 0;

  std::uint64_t bit = 33;
  while (!(d & 0xf) && bit >= 4) {
    d >>= 4;
    bit -= 4;
  }
  if (d == 0) {
    return 0;
  }
  while (r) {
    std::uint64_t shift = detail::clz(r);
    if (shift > bit) {
      shift = bit;
    }
    r <<= shift;
    bit -= shift;

    std::uint64_t div = r / d;
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

inline constexpr fixed constexpr_div(const fixed& a, const fixed& b) {
  std::int64_t sign = detail::fixed_sgn(a.value_, b.value_);
  std::uint64_t r = detail::fixed_abs(a.value_);
  std::uint64_t d = detail::fixed_abs(b.value_);
  std::uint64_t q = 0;

  std::uint64_t bit = 33;
  while (!(d & 0xf) && bit >= 4) {
    d >>= 4;
    bit -= 4;
  }
  if (d == 0) {
    return 0;
  }
  while (r) {
    std::uint64_t shift = detail::clz_constexpr(r);
    if (shift > bit) {
      shift = bit;
    }
    r <<= shift;
    bit -= shift;

    std::uint64_t div = r / d;
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
  if (f.value_ < 0) {
    o << "-";
  }

  std::uint64_t v = detail::fixed_abs(f.value_);
  o << (v >> 32) << ".";
  double d = 0;
  double val = 0.5;
  for (std::size_t i = 0; i < 32; ++i) {
    if (v & (std::int64_t{1} << (31 - i))) {
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
constexpr fixed tenth = constexpr_div(1_fx, 10);
constexpr fixed hundredth = constexpr_div(1_fx, 100);
constexpr fixed half = 1_fx >> 1;
constexpr fixed quarter = 1_fx >> 2;
constexpr fixed eighth = 1_fx >> 4;
constexpr fixed sixteenth = 1_fx >> 8;
}  // namespace fixed_c

inline constexpr fixed abs(const fixed& f) {
  return fixed::from_internal(detail::fixed_abs(f.value_));
}

inline fixed sqrt(const fixed& f) {
  if (f.value_ <= 0) {
    return 0;
  }
  if (f < 1) {
    return 1 / sqrt(1 / f);
  }

  const fixed a = f / 2;
  constexpr fixed half = 1_fx >> 1;
  constexpr fixed bound = 1_fx >> 10;

  auto r = fixed::from_internal(f.value_ >> ((32 - detail::clz(f.value_)) / 2));
  for (std::size_t n = 0; r && n < 8; ++n) {
    r = r * half + a / r;
    if (fixed::from_internal(detail::fixed_abs((r * r).value_) - f.value_) < bound) {
      break;
    }
  }
  return r;
}

inline constexpr fixed sin(const fixed& f) {
  auto angle = fixed::from_internal(detail::fixed_abs(f.value_) % (2 * fixed_c::pi).value_);

  if (angle > fixed_c::pi) {
    angle -= 2 * fixed_c::pi;
  }

  fixed angle2 = angle * angle;
  fixed out = angle;

  constexpr fixed r6 = constexpr_div(1_fx, 6);
  constexpr fixed r120 = constexpr_div(1_fx, 120);
  constexpr fixed r5040 = constexpr_div(1_fx, 5040);
  constexpr fixed r362880 = constexpr_div(1_fx, 362880);
  constexpr fixed r39916800 = constexpr_div(1_fx, 39916800);

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

  return f.value_ < 0 ? -out : out;
}

inline constexpr fixed cos(const fixed& f) {
  return sin(f + fixed_c::pi / 2);
}

inline constexpr fixed atan2(const fixed& y, const fixed& x) {
  auto ay = abs(y);
  fixed angle;

  if (x.value_ >= 0) {
    if (x + ay == 0) {
      return -fixed_c::pi / 4;
    }
    fixed r = (x - ay) / (x + ay);
    fixed r3 = r * r * r;
    angle = fixed::from_internal(0x32400000) * r3 - fixed::from_internal(0xfb500000) * r +
        fixed_c::pi / 4;
  } else {
    if (x - ay == 0) {
      return -3 * fixed_c::pi / 4;
    }
    fixed r = (x + ay) / (ay - x);
    fixed r3 = r * r * r;
    angle = fixed::from_internal(0x32400000) * r3 - fixed::from_internal(0xfb500000) * r +
        3 * fixed_c::pi / 4;
  }
  return y.value_ < 0 ? -angle : angle;
}

#endif
