#ifndef II_GAME_LOGIC_GEOMETRY_LEGACY_LINE_H
#define II_GAME_LOGIC_GEOMETRY_LEGACY_LINE_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/sim/io/render.h"

namespace ii::geom {
inline namespace legacy {

struct line_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  glm::vec4 colour{0.f};

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    std::invoke(f, *t.translate(a), *t.translate(b), colour, 1.f);
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    std::invoke(f,
                render::shape::line(to_float(*t.translate(a)), to_float(*t.translate(b)), colour));
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t.translate((a + b) / 2), colour);
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

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed AX, fixed AY, fixed BX, fixed BY, glm::vec4 Colour>
using line = constant<make_line(vec2{AX, AY}, vec2{BX, BY}, Colour)>;
template <fixed AX, fixed AY, fixed BX, fixed BY, std::size_t ParameterIndex>
using line_colour_p =
    line_eval<constant_vec2<AX, AY>, constant_vec2<BX, BY>, parameter<ParameterIndex>>;

}  // namespace legacy
}  // namespace ii::geom

#endif