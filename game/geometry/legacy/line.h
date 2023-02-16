#ifndef II_GAME_GEOMETRY_LEGACY_LINE_H
#define II_GAME_GEOMETRY_LEGACY_LINE_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"

namespace ii::geom::legacy {

struct line_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  cvec4 colour{0.f};
  unsigned char index = 0;
  render::flag r_flags = render::flag::kNone;

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr line_data make_line(const vec2& a, const vec2& b, const cvec4& colour,
                              unsigned char index, render::flag r_flags) {
  return {{}, a, b, colour, index, r_flags};
}

template <Expression<vec2> A, Expression<vec2> B, Expression<cvec4> Colour,
          Expression<unsigned char> Index = constant<0>,
          Expression<render::flag> RFlags = constant<render::flag::kNone>>
struct line_eval {};

template <Expression<vec2> A, Expression<vec2> B, Expression<cvec4> Colour,
          Expression<unsigned char> Index, Expression<render::flag> RFlags>
constexpr auto evaluate(line_eval<A, B, Colour, Index, RFlags>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   cvec4{evaluate(Colour{}, params)},
                   static_cast<unsigned char>(evaluate(Index{}, params)),
                   render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed AX, fixed AY, fixed BX, fixed BY, cvec4 Colour, unsigned char Index = 0,
          render::flag RFlags = render::flag::kNone>
using line = constant<make_line(vec2{AX, AY}, vec2{BX, BY}, Colour, Index, RFlags)>;
template <fixed AX, fixed AY, fixed BX, fixed BY, std::size_t ParameterIndex,
          unsigned char Index = 0, render::flag RFlags = render::flag::kNone>
using line_colour_p = line_eval<constant_vec2<AX, AY>, constant_vec2<BX, BY>,
                                parameter<ParameterIndex>, constant<Index>, constant<RFlags>>;

}  // namespace ii::geom::legacy

#endif