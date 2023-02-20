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

template <typename T>
struct value {
  constexpr value(const T& x) : v{x} {}
  constexpr value(key k) : v{k} {}
  std::variant<T, key> v;

  constexpr T operator()(const parameter_set& parameters) const {
    if (const auto* x = std::get_if<T>(&v)) {
      return *x;
    }
    auto k = *std::get_if<key>(&v);
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
};

}  // namespace ii::geom2

#endif
