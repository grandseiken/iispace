#ifndef II_GAME_GEOM2_VALUE_PARAMETERS_H
#define II_GAME_GEOM2_VALUE_PARAMETERS_H
#include "game/common/math.h"
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace ii::geom2 {

enum class key : unsigned char {};
using parameter_value = std::variant<bool, std::uint32_t, fixed, vec2, float, cvec4>;

struct parameter_set {
  std::unordered_map<key, parameter_value> map;

  void clear() { map.clear(); }

  template <typename T>
  void add(key k, const T& value) {
    if constexpr (std::is_enum_v<T>) {
      map.emplace(k,
                  parameter_value{std::in_place_type_t<std::uint32_t>{},
                                  static_cast<std::uint32_t>(value)});
    } else {
      map.emplace(k, parameter_value{std::in_place_type_t<T>{}, value});
    }
  }
};

template <typename T>
struct value {
  value(const T& x) : v{x} {}
  value(key k) : v{k} {}
  std::variant<T, key> v;

  T operator()(const parameter_set& parameters) const {
    if (const auto* x = std::get_if<T>(&v)) {
      return *x;
    }
    auto* k = std::get_if<key>(&v);
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
