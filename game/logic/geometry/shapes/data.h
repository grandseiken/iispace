#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_DATA_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_DATA_H
#include <glm/glm.hpp>

namespace ii::geom {
inline namespace shapes {

struct line_style {
  glm::vec4 colour{0.f};
  float z = 0.f;
  float width = 1.f;
};

struct fill_style {
  glm::vec4 colour{0.f};
  float z = 0.f;
};

constexpr line_style
sline(const glm::vec4& colour = glm::vec4{0.f}, float z = 0.f, float width = 1.f) {
  return {.colour = colour, .z = z, .width = width};
}

constexpr fill_style sfill(const glm::vec4& colour = glm::vec4{0.f}, float z = 0.f) {
  return {.colour = colour, .z = z};
}

}  // namespace shapes
}  // namespace ii::geom

#endif