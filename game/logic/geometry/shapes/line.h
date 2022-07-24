#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_LINE_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_LINE_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"

namespace ii::geom {

struct line_data : shape_data_base {
  using shape_data_base::check_point_legacy;
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  glm::vec4 colour{0.f};

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) const {
    std::invoke(f, t.translate(a).v, t.translate(b).v, colour);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) const {
    std::invoke(f, t.translate((a + b) / 2).v, colour);
  }
};

constexpr line_data make_line(const vec2& a, const vec2& b, const glm::vec4& colour) {
  return {{}, a, b, colour};
}

template <Expression<vec2> A, Expression<vec2> B, Expression<glm::vec4> Colour>
struct line_eval {};

template <Expression<vec2> A, Expression<vec2> B, Expression<glm::vec4> Colour>
constexpr auto evaluate(line_eval<A, B, Colour>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   glm::vec4{evaluate(Colour{}, params)});
}

template <fixed AX, fixed AY, fixed BX, fixed BY, glm::vec4 Colour>
using line = constant<make_line(vec2{AX, AY}, vec2{BX, BY}, Colour)>;

template <fixed AX, fixed AY, fixed BX, fixed BY, std::size_t ParameterIndex>
using line_colour_p =
    line_eval<constant_vec2<AX, AY>, constant_vec2<BX, BY>, parameter<ParameterIndex>>;

}  // namespace ii::geom

#endif