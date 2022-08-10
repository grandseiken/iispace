#ifndef II_GAME_COMMON_MATH_H
#define II_GAME_COMMON_MATH_H
#include "game/common/fix32.h"
#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>

inline constexpr glm::vec4 colour_hue(float h, float l = .5f, float s = 1.f) {
  return {h, s, l, 1.f};
}

inline constexpr glm::vec4 colour_hue360(std::uint32_t h, float l = .5f, float s = 1.f) {
  return colour_hue(h / 360.f, l, s);
}

using vec2 = glm::vec<2, fixed>;

inline constexpr vec2 operator*(const vec2& a, fixed b) {
  return glm::operator*(a, b);
}

inline constexpr vec2 operator*(fixed a, const vec2& b) {
  return glm::operator*(a, b);
}

inline constexpr vec2 operator/(const vec2& a, fixed b) {
  return glm::operator/(a, b);
}

template <typename T>
inline constexpr T length_squared(const glm::vec<2, T>& v) {
  return v.x * v.x + v.y * v.y;
}

template <typename T>
inline constexpr T length(const glm::vec<2, T>& v) {
  using std::sqrt;
  return sqrt(v.x * v.x + v.y * v.y);
}

template <typename T>
inline constexpr T angle(const glm::vec<2, T>& v) {
  using std::atan2;
  return atan2(v.y, v.x);
}

template <typename T>
inline constexpr glm::vec<2, T> normalise(const glm::vec<2, T>& v) {
  using std::sqrt;
  auto d = length_squared(v);
  if (!d) {
    return v;
  }
  return v / sqrt(d);
}

template <typename T>
inline constexpr glm::vec<2, T> from_polar(T theta, T length) {
  using std::cos;
  using std::sin;
  return length * glm::vec<2, T>{cos(theta), sin(theta)};
}

template <typename T>
inline constexpr glm::vec<2, T> rotate(const glm::vec<2, T>& v, T theta) {
  using std::cos;
  using std::sin;
  if (!theta) {
    return v;
  }
  auto c = cos(theta);
  auto s = sin(theta);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

template <typename T>
inline constexpr glm::vec<2, T> rotate_legacy(const glm::vec<2, T>& v, T theta) {
  using std::sqrt;
  return theta ? from_polar(angle(v) + theta, sqrt(length_squared(v))) : v;
}

inline constexpr glm::vec2 to_float(const vec2& a) {
  return {a.x.to_float(), a.y.to_float()};
}

inline constexpr fixed normalise_angle(fixed a) {
  return a > 2 * fixed_c::pi ? a - 2 * fixed_c::pi : a < 0 ? a + 2 * fixed_c::pi : a;
}

inline constexpr fixed angle_diff(fixed from, fixed to) {
  if (to == from) {
    return 0_fx;
  }
  if (to > from) {
    return to - from <= fixed_c::pi ? to - from : to - (2 * fixed_c::pi + from);
  }
  return from - to <= fixed_c::pi ? to - from : 2 * fixed_c::pi + to - from;
}

#endif
