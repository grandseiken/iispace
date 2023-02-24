#ifndef II_GAME_COMMON_RECT_H
#define II_GAME_COMMON_RECT_H
#include "game/common/fix32.h"
#include <glm/gtx/extended_min_max.hpp>
#include <glm/vec2.hpp>
#include <cstdint>

namespace ii {

template <typename T>
struct basic_rect {
  using value_type = T;
  using vec_type = glm::vec<2, T>;

  basic_rect() : position{0, 0}, size{0, 0} {}
  basic_rect(const vec_type& position, const vec_type& size)
  : position{position}, size{glm::max(vec_type{0, 0}, size)} {}
  explicit basic_rect(const vec_type& size) : basic_rect({0, 0}, size) {}
  template <typename U>
  explicit basic_rect(const basic_rect<U>& rect) : position{rect.position}, size{rect.size} {}

  vec_type position;
  vec_type size;

  bool empty() const { return !size.x && !size.y; }
  bool operator==(const basic_rect&) const = default;
  bool operator!=(const basic_rect&) const = default;

  bool contains(const vec_type& p) const {
    return glm::all(glm::greaterThanEqual(p, min())) && glm::all(glm::lessThan(p, max()));
  }

  vec_type min() const { return position; }
  vec_type max() const { return position + size; }
  basic_rect size_rect() const { return basic_rect{size}; }

  vec_type relative(const vec_type& p) const { return p - position; }
  basic_rect relative(const basic_rect& r) const { return r - position; }

  basic_rect intersect(const basic_rect& r) const {
    auto p = glm::max(min(), r.min());
    return {p, glm::min(max(), r.max()) - p};
  }

  basic_rect extend(const vec_type& border) const {
    return {position - border, size + T{2} * border};
  }
  basic_rect extend_min(const vec_type& v) const { return {{position - v}, {size + v}}; }
  basic_rect extend_max(const vec_type& v) const { return {position, {size + v}}; }

  basic_rect contract(const vec_type& border) const {
    auto b = glm::min(border, size / T{2});
    return {position + b, size - T{2} * b};
  }

  basic_rect contract_min(const vec_type& v) const { return intersect(*this + v); }
  basic_rect contract_max(const vec_type& v) const { return intersect(*this - v); }

  basic_rect operator+(const vec_type& v) const { return basic_rect{position + v, size}; }
  basic_rect operator-(const vec_type& v) const { return basic_rect{position - v, size}; }

  basic_rect& operator+=(const vec_type& v) {
    position += v;
    return *this;
  }

  basic_rect& operator-=(const vec_type& v) {
    position -= v;
    return *this;
  }
};

using rect = basic_rect<fixed>;
using irect = basic_rect<std::int32_t>;
using frect = basic_rect<float>;

}  // namespace ii

#endif
