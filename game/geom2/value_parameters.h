#ifndef II_GAME_GEOM2_VALUE_PARAMETERS_H
#define II_GAME_GEOM2_VALUE_PARAMETERS_H
#include "game/common/math.h"
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace ii::geom2 {

enum class parameter_key : unsigned char {};
using parameter_value = std::variant<bool, std::uint32_t, fixed, vec2, float, cvec4>;

struct parameter_set {
  std::unordered_map<parameter_key, parameter_value> map;

  template <typename T>
  void add(parameter_key k, const T& value) {
    if constexpr (std::is_enum_v<T>) {
      map.emplace(k, static_cast<std::uint32_t>(value));
    } else {
      map.emplace(k, value);
    }
  }
};

template <typename T>
struct value {
  value(const T& x) : v{x} {}
  value(parameter_key k) : v{k} {}
  std::variant<T, parameter_key> v;

  T operator()(const parameter_set& parameters) const {
    if (const auto* x = std::get_if<T>(&v)) {
      return *x;
    }
    auto* k = std::get_if<parameter_key>(&v);
    if (auto it = parameters.map.find(*k); it != parameters.map.end()) {
      if constexpr (std::is_enum_v<T>) {
        if (const auto* x = std::get_if<std::uint32_t>(&it->second)) {
          return static_cast<T>(*x);
        }
      } else {
        if (const auto* x = std::get_if<T>(&it->second)) {
          return *x;
        }
      }
    }
    return T{0};
  };
};

}  // namespace ii::geom2

#endif
