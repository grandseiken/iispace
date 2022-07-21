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
  using namespace std;
  return sqrt(v.x * v.x + v.y * v.y);
}

template <typename T>
inline constexpr T angle(const glm::vec<2, T>& v) {
  using namespace std;
  return atan2(v.y, v.x);
}

template <typename T>
inline constexpr glm::vec<2, T> normalise(const glm::vec<2, T>& v) {
  using namespace std;
  if (v == glm::vec<2, T>{0}) {
    return v;
  }
  return v / sqrt(length_squared(v));
}

template <typename T>
inline constexpr glm::vec<2, T> from_polar(T theta, T length) {
  using namespace std;
  return length * glm::vec<2, T>{cos(theta), sin(theta)};
}

template <typename T>
inline constexpr glm::vec<2, T> rotate(const glm::vec<2, T>& v, T theta) {
  using namespace std;
  return theta ? from_polar(angle(v) + theta, sqrt(length_squared(v))) : v;
}

inline constexpr glm::vec2 to_float(const vec2& a) {
  return {a.x.to_float(), a.y.to_float()};
}

inline constexpr fixed normalise_angle(fixed a) {
  return a > 2 * fixed_c::pi ? a - 2 * fixed_c::pi : a < 0 ? a + 2 * fixed_c::pi : a;
}

#endif
