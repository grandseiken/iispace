#ifndef II_GAME_GEOMETRY_SHAPES_DATA_H
#define II_GAME_GEOMETRY_SHAPES_DATA_H
#include "game/common/math.h"
#include <optional>

namespace ii::geom {

struct line_style {
  cvec4 colour0{0.f};
  cvec4 colour1{0.f};
  float z = 0.f;
  float width = 1.f;
  unsigned char index = 0;
};

struct fill_style {
  cvec4 colour0{0.f};
  cvec4 colour1{0.f};
  float z = 0.f;
  unsigned char index = 0;
};

constexpr line_style
sline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {.colour0 = colour, .colour1 = colour, .z = z, .width = width, .index = index};
}

constexpr line_style sline(const cvec4& colour0, const cvec4& colour1, float z = 0.f,
                           float width = 1.f, unsigned char index = 0) {
  return {.colour0 = colour0, .colour1 = colour1, .z = z, .width = width, .index = index};
}

constexpr fill_style
sfill(const cvec4& colour = cvec4{0.f}, float z = 0.f, unsigned char index = 0) {
  return {.colour0 = colour, .colour1 = colour, .z = z, .index = index};
}

constexpr fill_style
sfill(const cvec4& colour0, const cvec4& colour1, float z = 0.f, unsigned char index = 0) {
  return {.colour0 = colour0, .colour1 = colour1, .z = z, .index = index};
}

}  // namespace ii::geom

#endif