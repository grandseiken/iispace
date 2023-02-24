#ifndef II_GAME_COMMON_MATH_H
#define II_GAME_COMMON_MATH_H
#include "game/common/fix32.h"
#include <glm/gtc/constants.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>

using vec2 = glm::vec<2, fixed>;
using ivec2 = glm::ivec2;
using uvec2 = glm::uvec2;
using fvec2 = glm::vec2;
using fvec4 = glm::vec4;
using cvec3 = glm::vec3;
using cvec4 = glm::vec4;

namespace detail {
template <typename T>
struct pi_helper {
  static inline constexpr T value = glm::pi<T>();
};
template <>
struct pi_helper<fixed> {
  static inline constexpr fixed value = fixed_c::pi;
};
}  // namespace detail

template <typename T>
constexpr T pi = detail::pi_helper<T>::value;

inline void hash_combine(std::size_t& seed, std::size_t v) {
  seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T>
inline constexpr T square(const T& x) {
  return x * x;
}

template <typename T>
inline constexpr T cube(const T& x) {
  return x * x * x;
}

template <typename T>
inline constexpr T fourth_power(const T& x) {
  return square(square(x));
}

template <typename T>
inline constexpr T fifth_power(const T& x) {
  return x * fourth_power(x);
}

inline constexpr vec2 operator*(const vec2& a, fixed b) {
  return glm::operator*(a, b);
}

inline constexpr vec2 operator*(fixed a, const vec2& b) {
  return glm::operator*(a, b);
}

inline constexpr vec2 operator/(const vec2& a, fixed b) {
  return glm::operator/(a, b);
}

// (Right) perpendicular vector.
inline constexpr vec2 perpendicular(const vec2& v) {
  return vec2{-v.y, v.x};
}

template <typename T>
constexpr T cdot(const glm::vec<2, T>& a, const glm::vec<2, T>& b) {
  return a.x * b.x + a.y * b.y;
}

template <typename T>
constexpr T length_squared(const glm::vec<2, T>& v) {
  return cdot(v, v);
}

template <typename T>
constexpr T length(const glm::vec<2, T>& v) {
  using std::sqrt;
  return sqrt(length_squared(v));
}

template <typename T>
constexpr T angle(const glm::vec<2, T>& v) {
  using std::atan2;
  return atan2(v.y, v.x);
}

template <glm::length_t N, typename T>
constexpr glm::vec<N, T> lerp(const glm::vec<N, T>& a, const glm::vec<N, T>& b, T t) {
  return t * b + (T{1} - t) * a;
}

template <glm::length_t N, typename T>
constexpr glm::vec<N, T>
rc_smooth(const glm::vec<N, T>& v, const glm::vec<N, T>& target, const T& coefficient) {
  return coefficient * (v - target) + target;
}

template <typename T>
constexpr T rc_smooth(const T& v, const T& target, const T& coefficient) {
  return coefficient * (v - target) + target;
}

template <typename T>
constexpr glm::vec<2, T> normalise(const glm::vec<2, T>& v) {
  using std::sqrt;
  auto d = length_squared(v);
  if (!d) {
    return v;
  }
  return v / sqrt(d);
}

template <typename T>
constexpr glm::vec<2, T> from_polar(T theta, T length) {
  using std::cos;
  using std::sin;
  return length * glm::vec<2, T>{cos(theta), sin(theta)};
}

inline constexpr vec2 from_polar_legacy(fixed theta, fixed length) {
  return length * vec2{cos_legacy(theta), sin_legacy(theta)};
}

// (Right) rotation.
template <typename T>
constexpr glm::vec<2, T> rotate(const glm::vec<2, T>& v, T theta) {
  using std::cos;
  using std::sin;
  if (!theta) {
    return v;
  }
  auto c = cos(theta);
  auto s = sin(theta);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

inline constexpr vec2 rotate_legacy(const vec2& v, fixed theta) {
  return theta ? from_polar_legacy(angle(v) + theta, sqrt(length_squared(v))) : v;
}

inline constexpr fvec2 to_float(const vec2& a) {
  return {a.x.to_float(), a.y.to_float()};
}

inline constexpr fixed normalise_angle(fixed a) {
  return a > 2 * pi<fixed> ? a - 2 * pi<fixed> : a < 0 ? a + 2 * pi<fixed> : a;
}

inline constexpr float normalise_angle(float a) {
  return a > 2.f * pi<float> ? a - 2.f * pi<float> : a < 0.f ? a + 2.f * pi<float> : a;
}

inline constexpr fixed angle_diff(fixed from, fixed to) {
  if (to == from) {
    return 0_fx;
  }
  if (to > from) {
    return to - from <= pi<fixed> ? to - from : to - (2 * pi<fixed> + from);
  }
  return from - to <= pi<fixed> ? to - from : 2 * pi<fixed> + to - from;
}

inline constexpr float angle_diff(float from, float to) {
  if (to == from) {
    return 0.f;
  }
  if (to > from) {
    return to - from <= pi<float> ? to - from : to - (2.f * pi<float> + from);
  }
  return from - to <= pi<float> ? to - from : 2.f * pi<float> + to - from;
}

template <std::size_t N, typename T>
struct vec_hash {
  std::size_t operator()(const glm::vec<N, T>& v) const {
    std::size_t r = 0;
    hash_combine(r, std::hash<T>{}(v.x));
    if constexpr (N >= 2) {
      hash_combine(r, std::hash<T>{}(v.y));
    }
    if constexpr (N >= 3) {
      hash_combine(r, std::hash<T>{}(v.z));
    }
    if constexpr (N >= 4) {
      hash_combine(r, std::hash<T>{}(v.w));
    }
    return r;
  }
};

#endif
