#ifndef II_GAME_COMMON_Z_H
#define II_GAME_COMMON_Z_H
#include "game/common/fix32.h"
#include <glm/glm.hpp>
#include <cmath>
#include <cstdint>

inline glm::vec4 colour_hue(float h, float l = .5f, float s = 1.f) {
  return {h, s, l, 1.f};
}

inline glm::vec4 colour_hue360(std::uint32_t h, float l = .5f, float s = 1.f) {
  return colour_hue(h / 360.f, l, s);
}

using vec2 = glm::vec<2, fixed>;

inline vec2 operator*(const vec2& a, fixed b) {
  return glm::operator*(a, b);
}

inline vec2 operator*(fixed a, const vec2& b) {
  return glm::operator*(a, b);
}

inline vec2 operator/(const vec2& a, fixed b) {
  return glm::operator/(a, b);
}

template <typename T>
inline T length_squared(const glm::vec<2, T>& v) {
  return dot(v, v);
}

template <typename T>
inline T angle(const glm::vec<2, T>& v) {
  return atan2(v.y, v.x);
}

template <typename T>
inline glm::vec<2, T> normalise(const glm::vec<2, T>& v) {
  if (v == glm::vec<2, T>{0}) {
    return v;
  }
  return v / length(v);
}

template <typename T>
inline glm::vec<2, T> from_polar(T theta, T length) {
  return length * glm::vec<2, T>{cos(theta), sin(theta)};
}

template <typename T>
inline glm::vec<2, T> rotate(const glm::vec<2, T>& v, T theta) {
  return theta ? from_polar(angle(v) + theta, length(v)) : v;
}

inline glm::vec2 to_float(const vec2& a) {
  return {a.x.to_float(), a.y.to_float()};
}

#endif
