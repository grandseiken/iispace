#ifndef II_GAME_COMMON_Z_H
#define II_GAME_COMMON_Z_H
#include "game/common/fix32.h"
#include <cmath>
#include <cstdint>
#include <vector>

typedef std::uint32_t colour_t;

namespace z {

colour_t colour_cycle(colour_t rgb, std::int32_t cycle);

}  // namespace z

constexpr float kPiFloat = 3.14159265358979323846264338327f;

template <typename T>
class vec2_t {
public:
  T x = 0;
  T y = 0;

  vec2_t() = default;

  vec2_t(const T& x, const T& y) : x{x}, y{y} {}

  T length_squared() const {
    return dot(*this);
  }

  T length() const {
    return sqrt(length_squared());
  }

  T angle() const {
    return atan2(y, x);
  }

  static vec2_t from_polar(const T& angle, const T& length) {
    return length * vec2_t{cos(angle), sin(angle)};
  }

  vec2_t normalised() const {
    T len = length();
    return len <= 0 ? *this : *this / len;
  }

  vec2_t rotated(const T& r_angle) const {
    return r_angle ? from_polar(angle() + r_angle, length()) : *this;
  }

  T dot(const vec2_t& v) const {
    return x * v.x + y * v.y;
  }

  vec2_t operator-() const {
    return vec2_t{} - *this;
  }

  vec2_t operator+(const vec2_t& v) const {
    return vec2_t{x + v.x, y + v.y};
  }

  vec2_t operator-(const vec2_t& v) const {
    return vec2_t{x - v.x, y - v.y};
  }

  vec2_t operator+=(const vec2_t& v) {
    return *this = *this + v;
  }

  vec2_t operator-=(const vec2_t& v) {
    return *this = *this - v;
  }

  vec2_t operator*(const T& t) const {
    return vec2_t{x * t, y * t};
  }

  vec2_t operator/(const T& t) const {
    return vec2_t{x / t, y / t};
  }

  vec2_t operator*=(const T& t) {
    return *this = *this * t;
  }

  vec2_t operator/=(const T& t) {
    return *this = *this / t;
  }

  bool operator==(const vec2_t& v) const {
    return x == v.x && y == v.y;
  }

  bool operator!=(const vec2_t& v) const {
    return !(*this == v);
  }

  bool operator<(const vec2_t& v) const {
    return x < v.x && y < v.y;
  }

  bool operator>(const vec2_t& v) const {
    return x > v.x && y > v.y;
  }

  bool operator<=(const vec2_t& v) const {
    return x <= v.x && y <= v.y;
  }

  bool operator>=(const vec2_t& v) const {
    return x >= v.x && y >= v.y;
  }

  vec2_t& operator=(const vec2_t& v) {
    x = v.x;
    y = v.y;
    return *this;
  }
};

namespace std {
template <typename T>
vec2_t<T> max(const vec2_t<T>& a, const vec2_t<T>& b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y)};
}

template <typename T>
vec2_t<T> min(const vec2_t<T>& a, const vec2_t<T>& b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y)};
}
}  // namespace std

template <typename T>
vec2_t<T> operator*(const T& t, const vec2_t<T>& v) {
  return {t * v.x, t * v.y};
}

template <typename T>
vec2_t<T> operator/(const T& t, const vec2_t<T>& v) {
  return {t / v.x, t / v.y};
}

typedef vec2_t<fixed> vec2;
typedef vec2_t<std::int32_t> ivec2;
typedef vec2_t<float> fvec2;

inline fvec2 to_float(const vec2& a) {
  return {a.x.to_float(), a.y.to_float()};
}

#endif
