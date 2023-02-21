#ifndef II_GAME_GEOM2_VALUE_PARAMETERS_H
#define II_GAME_GEOM2_VALUE_PARAMETERS_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include "game/common/variant_switch.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace ii::geom2 {

enum class key : unsigned char {};
using parameter_value = std::variant<bool, std::uint32_t, fixed, vec2, float, cvec4>;

}  // namespace ii::geom2

namespace ii {
template <>
struct integral_enum<geom2::key> : std::true_type {};
}  // namespace ii

namespace ii::geom2 {

// TODO: allow transformation matrices of some kind?
// TODO: possibly allow indexed parameters?
// TODO: any way to type-check parameters (or maybe convert if possible)?
struct parameter_set {
  std::array<parameter_value, 256> map;

  template <typename T>
  parameter_set& add(key k, const T& value) {
    if constexpr (std::is_enum_v<T>) {
      map[static_cast<std::uint32_t>(k)] =
          parameter_value{std::in_place_type_t<std::uint32_t>{}, static_cast<std::uint32_t>(value)};
    } else {
      map[static_cast<std::uint32_t>(k)] = parameter_value{std::in_place_type_t<T>{}, value};
    }
    return *this;
  }
};

// TODO: allow more arbitrary expressions? Also note, expressions kind of slow. Optimize somehow?
template <typename T>
struct value {
  constexpr value() : v{T{0}} {}
  constexpr value(const T& x) : v{x} {}
  constexpr value(key k) : v{k} {}

  static value multiply(const T& x, key k) {
    value v;
    v.v = multiply_t{x, k};
    return v;
  }

  static value compare(parameter_value x, key k, const T& true_value, const T& false_value) {
    value v;
    v.v = compare_t{x, k, true_value, false_value};
    return v;
  }

  struct multiply_t {
    T x;
    key k;
  };

  struct compare_t {
    parameter_value x;
    key k;
    T true_value;
    T false_value;
  };

  std::variant<T, key, multiply_t, compare_t> v;

  constexpr T operator()(const parameter_set& parameters) const {
    auto get_key = [&](key k) -> T {
      const auto& v = parameters.map[static_cast<std::uint32_t>(k)];
      if constexpr (std::is_enum_v<T>) {
        if (v.index() == variant_index<parameter_value, std::uint32_t>) {
          return static_cast<T>(*std::get_if<std::uint32_t>(&v));
        }
      } else {
        if (v.index() == variant_index<parameter_value, T>) {
          return *std::get_if<T>(&v);
        }
      }
      return T{0};
    };

    switch (v.index()) {
      VARIANT_CASE_GET(T, v, x) {
        return x;
      }

      VARIANT_CASE_GET(key, v, x) {
        return get_key(x);
      }

      VARIANT_CASE_GET(multiply_t, v, x) {
        if constexpr (!std::is_enum_v<T>) {
          return x.x * get_key(x.k);
        } else {
          break;
        }
      }

      VARIANT_CASE_GET(compare_t, v, x) {
        return x.x == parameters.map[static_cast<std::uint32_t>(x.k)] ? x.true_value
                                                                      : x.false_value;
      }
    }
    return T{0};
  };
};

template <typename T>
inline value<T> compare(parameter_value x, key k, const T& true_value, const T& false_value) {
  return value<T>::compare(x, k, true_value, false_value);
}
inline value<bool> compare(parameter_value x, key k) {
  return value<bool>::compare(x, k, true, false);
}
template <typename T>
inline value<T> multiply(const T& x, key k) {
  return value<T>::multiply(x, k);
}

}  // namespace ii::geom2

#endif
