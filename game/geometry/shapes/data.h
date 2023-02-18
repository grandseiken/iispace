#ifndef II_GAME_GEOMETRY_SHAPES_DATA_H
#define II_GAME_GEOMETRY_SHAPES_DATA_H
#include "game/common/math.h"
#include "game/geom2/resolve.h"
#include <optional>

namespace ii::geom {

using line_style = geom2::resolve_result::line_style;
using fill_style = geom2::resolve_result::fill_style;

constexpr line_style sline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f) {
  return {.colour0 = colour, .colour1 = colour, .z = z, .width = width};
}

constexpr line_style
sline(const cvec4& colour0, const cvec4& colour1, float z = 0.f, float width = 1.f) {
  return {.colour0 = colour0, .colour1 = colour1, .z = z, .width = width};
}

constexpr fill_style sfill(const cvec4& colour = cvec4{0.f}, float z = 0.f) {
  return {.colour0 = colour, .colour1 = colour, .z = z};
}

constexpr fill_style sfill(const cvec4& colour0, const cvec4& colour1, float z = 0.f) {
  return {.colour0 = colour0, .colour1 = colour1, .z = z};
}

}  // namespace ii::geom

#endif