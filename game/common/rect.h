#ifndef II_GAME_COMMON_RECT_H
#define II_GAME_COMMON_RECT_H
#include <glm/glm.hpp>

namespace ii {

struct rect {
  rect() : position{0, 0}, size{0, 0} {}
  rect(const glm::ivec2& position, const glm::ivec2& size)
  : position{position}, size{glm::max(glm::ivec2{0, 0}, size)} {}
  explicit rect(const glm::ivec2& size) : rect({0, 0}, size) {}

  glm::ivec2 position;
  glm::ivec2 size;

  bool empty() const {
    return !size.x && !size.y;
  }

  bool contains(const glm::ivec2& p) const {
    return glm::all(glm::greaterThanEqual(p, min())) && glm::all(glm::lessThan(p, max()));
  }

  glm::ivec2 min() const {
    return position;
  }

  glm::ivec2 max() const {
    return position + size;
  }

  rect size_rect() const {
    return rect{size};
  }

  glm::ivec2 relative(const glm::ivec2& p) const {
    return p - position;
  }

  rect relative(const rect& r) const {
    return r - position;
  }

  rect intersect(const rect& r) const {
    auto p = glm::max(min(), r.min());
    return {p, glm::min(max(), r.max()) - p};
  }

  rect extend(const glm::ivec2& border) const {
    return {position - border, size + 2 * border};
  }

  rect extend_min(const glm::ivec2& v) const {
    return {{position - v}, {size + v}};
  }

  rect extend_max(const glm::ivec2& v) const {
    return {position, {size + v}};
  }

  rect contract(const glm::ivec2& border) const {
    auto b = glm::min(border, size / 2);
    return {position + b, size - 2 * b};
  }

  rect contract_min(const glm::ivec2& v) const {
    return intersect(*this + v);
  }

  rect contract_max(const glm::ivec2& v) const {
    return intersect(*this - v);
  }

  rect operator+(const glm::ivec2& v) const {
    return rect{position + v, size};
  }

  rect operator-(const glm::ivec2& v) const {
    return rect{position - v, size};
  }

  rect& operator+=(const glm::ivec2& v) {
    position += v;
    return *this;
  }

  rect& operator-=(const glm::ivec2& v) {
    position -= v;
    return *this;
  }
};

}  // namespace ii

#endif