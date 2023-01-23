#ifndef II_GAME_GEOMETRY_LEGACY_LINE_H
#define II_GAME_GEOMETRY_LEGACY_LINE_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"

namespace ii::geom {
inline namespace legacy {

struct line_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  cvec4 colour{0.f};
  unsigned char index = 0;

  constexpr void iterate(iterate_lines_t, const Transform auto& t, LineFunction auto&& f) const {
    std::invoke(f, *t.translate(a), *t.translate(b), colour, 1.f, 0.f);
  }

  constexpr void iterate(iterate_shapes_t, const Transform auto& t, ShapeFunction auto&& f) const {
    std::invoke(f,
                render::shape::line(to_float(*t.translate(a)), to_float(*t.translate(b)), colour,
                                    0.f, 1.f, index));
  }

  constexpr void
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t.translate((a + b) / 2), 0_fx, colour, colour::kZero);
  }
};

constexpr line_data
make_line(const vec2& a, const vec2& b, const cvec4& colour, unsigned char index) {
  return {{}, a, b, colour, index};
}

template <Expression<vec2> A, Expression<vec2> B, Expression<cvec4> Colour,
          Expression<unsigned char> Index = constant<0>>
struct line_eval {};

template <Expression<vec2> A, Expression<vec2> B, Expression<cvec4> Colour,
          Expression<unsigned char> Index>
constexpr auto evaluate(line_eval<A, B, Colour, Index>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   cvec4{evaluate(Colour{}, params)},
                   static_cast<unsigned char>(evaluate(Index{}, params)));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed AX, fixed AY, fixed BX, fixed BY, cvec4 Colour, unsigned char Index = 0>
using line = constant<make_line(vec2{AX, AY}, vec2{BX, BY}, Colour, Index)>;
template <fixed AX, fixed AY, fixed BX, fixed BY, std::size_t ParameterIndex,
          unsigned char Index = 0>
using line_colour_p = line_eval<constant_vec2<AX, AY>, constant_vec2<BX, BY>,
                                parameter<ParameterIndex>, constant<Index>>;

}  // namespace legacy
}  // namespace ii::geom

#endif