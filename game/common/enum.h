#ifndef II_GAME_COMMON_ENUM_H
#define II_GAME_COMMON_ENUM_H
#include <type_traits>

namespace ii {

template <typename, typename = void>
struct integral_enum : std::false_type {};

template <typename, typename = void>
struct bitmask_enum : std::false_type {};

template <typename E>
concept Enum = std::is_enum_v<E> && requires(E e) {
                                      static_cast<std::underlying_type_t<E>>(e);
                                      e = E{std::underlying_type_t<E>{}};
                                      E{std::underlying_type_t<E>{}};
                                    };

template <typename E>
concept IntegralEnum = Enum<E> && integral_enum<E>::value;

template <typename E>
concept BitmaskEnum =
    Enum<E> && std::is_unsigned_v<std::underlying_type_t<E>> && bitmask_enum<E>::value;

template <Enum E>
constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

template <typename E>
requires IntegralEnum<E> || BitmaskEnum<E>
constexpr bool operator!(E e) {
  return !to_underlying(e);
}

template <typename E>
requires IntegralEnum<E> || BitmaskEnum<E>
constexpr std::underlying_type_t<E> operator+(E e) {
  return to_underlying(e);
}

template <IntegralEnum E>
constexpr E operator-(E e) {
  return E{static_cast<std::underlying_type_t<E>>(-to_underlying(e))};
}

template <IntegralEnum E>
constexpr E operator++(E& e) {
  auto v = to_underlying(e);
  return e = E{static_cast<std::underlying_type_t<E>>(++v)};
}

template <IntegralEnum E>
constexpr E operator--(E& e) {
  auto v = to_underlying(e);
  return e = E{static_cast<std::underlying_type_t<E>>(--v)};
}

template <IntegralEnum E>
constexpr E operator++(E& e, int) {
  auto v = e;
  ++e;
  return v;
}

template <IntegralEnum E>
constexpr E operator--(E& e, int) {
  auto v = e;
  --e;
  return v;
}

template <IntegralEnum E>
constexpr E operator+(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) + to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator-(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) - to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator*(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) * to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator/(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) / to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator%(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) % to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator<<(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) << to_underlying(b))};
}

template <IntegralEnum E>
constexpr E operator>>(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) >> to_underlying(b))};
}

template <IntegralEnum E>
constexpr E& operator+=(E& a, E b) {
  return a = a + b;
}

template <IntegralEnum E>
constexpr E& operator-=(E& a, E b) {
  return a = a - b;
}

template <IntegralEnum E>
constexpr E& operator*=(E& a, E b) {
  return a = a * b;
}

template <IntegralEnum E>
constexpr E& operator/=(E& a, E b) {
  return a = a / b;
}

template <IntegralEnum E>
constexpr E& operator%=(E& a, E b) {
  return a = a % b;
}

template <IntegralEnum E>
constexpr E& operator<<=(E& a, E b) {
  return a = a << b;
}

template <IntegralEnum E>
constexpr E& operator>>=(E& a, E b) {
  return a = a >> b;
}

template <BitmaskEnum E>
constexpr E operator~(E e) {
  return E{static_cast<std::underlying_type_t<E>>(~to_underlying(e))};
}

template <BitmaskEnum E>
constexpr E operator&(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) & to_underlying(b))};
}

template <BitmaskEnum E>
constexpr E operator|(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) | to_underlying(b))};
}

template <BitmaskEnum E>
constexpr E operator^(E a, E b) {
  return E{static_cast<std::underlying_type_t<E>>(to_underlying(a) ^ to_underlying(b))};
}

template <BitmaskEnum E>
constexpr E& operator&=(E& a, E b) {
  return a = a & b;
}

template <BitmaskEnum E>
constexpr E& operator|=(E& a, E b) {
  return a = a | b;
}

template <BitmaskEnum E>
constexpr E& operator^=(E& a, E b) {
  return a = a ^ b;
}

}  // namespace ii

#endif