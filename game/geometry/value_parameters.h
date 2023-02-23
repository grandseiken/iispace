#ifndef II_GAME_GEOMETRY_VALUE_PARAMETERS_H
#define II_GAME_GEOMETRY_VALUE_PARAMETERS_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include "game/common/variant_switch.h"
#include <array>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace ii::geom {

enum class key : unsigned char {};
using parameter_value = std::variant<bool, std::uint32_t, fixed, vec2, float, cvec4>;

}  // namespace ii::geom

namespace ii {
template <>
struct integral_enum<geom::key> : std::true_type {};
}  // namespace ii

namespace ii::geom {

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

  template <typename T>
  T get(key k) const {
    const auto& v = map[static_cast<std::uint32_t>(k)];
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
  }
};

struct expression_base {
  virtual ~expression_base() = default;
};

template <typename T>
struct expression : expression_base {
  ~expression() override = default;
  virtual T operator()(const parameter_set& parameters) const = 0;
};

template <typename T>
struct e_constant : expression<T> {
  ~e_constant() override = default;
  e_constant(const T& x) : x{x} {}
  T operator()(const parameter_set&) const override { return x; }
  T x;
};

template <typename T>
struct e_parameter : expression<T> {
  ~e_parameter() override = default;
  e_parameter(key k) : k{k} {}
  T operator()(const parameter_set& parameters) const override { return parameters.get<T>(k); }
  key k;
};

template <typename T>
struct e_multiply : expression<T> {
  ~e_multiply() override = default;
  e_multiply(const expression<T>& a, const expression<T>& b) : a{&a}, b{&b} {}
  T operator()(const parameter_set& parameters) const override {
    return (*a)(parameters) * (*b)(parameters);
  }
  const expression<T>* a = nullptr;
  const expression<T>* b = nullptr;
};

template <typename T>
struct e_cmp_eq : expression<bool> {
  ~e_cmp_eq() override = default;
  e_cmp_eq(const expression<T>& a, const expression<T>& b) : a{&a}, b{&b} {}
  bool operator()(const parameter_set& parameters) const override {
    return (*a)(parameters) == (*b)(parameters);
  }
  const expression<T>* a = nullptr;
  const expression<T>* b = nullptr;
};

template <typename T>
struct e_ternary : expression<T> {
  ~e_ternary() override = default;
  e_ternary(const expression<bool>& t, const expression<T>& a, const expression<T>& b)
  : t{&t}, a{&a}, b{&b} {}
  T operator()(const parameter_set& parameters) const override {
    return (*t)(parameters) ? (*a)(parameters) : (*b)(parameters);
  }
  const expression<bool>* t = nullptr;
  const expression<T>* a = nullptr;
  const expression<T>* b = nullptr;
};

template <typename T>
struct value {
  constexpr value(const expression<T>& e) : v{&e} {}
  constexpr value(const T& x) : v{x} {}
  constexpr value(key k) : v{k} {}

  std::variant<T, key, const expression<T>*> v;

  constexpr T operator()(const parameter_set& parameters) const {
    switch (v.index()) {
      VARIANT_CASE_GET(T, v, x) {
        return x;
      }

      VARIANT_CASE_GET(key, v, k) {
        return parameters.get<T>(k);
      }

      VARIANT_CASE_GET(const expression<T>*, v, e) {
        return (*e)(parameters);
      }
    }
    return T{0};
  };
};

}  // namespace ii::geom

#endif
