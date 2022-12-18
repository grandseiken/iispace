#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_DATA_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_DATA_H
#include "game/common/math.h"

namespace ii::geom {
inline namespace shapes {

struct line_style {
  cvec4 colour{0.f};
  float z = 0.f;
  float width = 1.f;
  unsigned char index = 0;
};

struct fill_style {
  cvec4 colour{0.f};
  float z = 0.f;
  unsigned char index = 0;
};

constexpr line_style
sline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {.colour = colour, .z = z, .width = width, .index = index};
}

constexpr fill_style
sfill(const cvec4& colour = cvec4{0.f}, float z = 0.f, unsigned char index = 0) {
  return {.colour = colour, .z = z, .index = index};
}

}  // namespace shapes
}  // namespace ii::geom

#endif