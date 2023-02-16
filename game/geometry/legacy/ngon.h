#ifndef II_GAME_GEOMETRY_LEGACY_NGON_H
#define II_GAME_GEOMETRY_LEGACY_NGON_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom {
using render::ngon_style;
}  // namespace ii::geom

namespace ii::geom::legacy {

struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  cvec4 colour{0.f};
  unsigned char index = 0;
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;
  render::flag r_flags = render::flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  constexpr void
  iterate(iterate_check_collision_t it, const Transform auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point>(&it.check)) {
      if (length_squared(t.transform_ignore_rotation(c->v)) < square(radius)) {
        hit.add(flags & it.mask);
      }
    }
  }

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr ngon_data make_ngon(fixed radius, std::uint32_t sides, const cvec4& colour,
                              ngon_style style = ngon_style::kPolygon, unsigned char index = 0,
                              shape_flag flags = shape_flag::kNone,
                              render::flag r_flags = render::flag::kNone) {
  return {{}, radius, sides, colour, index, style, flags, r_flags};
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<cvec4> Colour,
          Expression<ngon_style> Style = constant<ngon_style::kPolygon>,
          Expression<unsigned char> Index = constant<0>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>,
          Expression<render::flag> RFlags = constant<render::flag::kNone>>
struct ngon_eval {};

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<cvec4> Colour,
          Expression<ngon_style> Style, Expression<unsigned char> Index,
          Expression<shape_flag> Flags, Expression<render::flag> RFlags>
constexpr auto
evaluate(ngon_eval<Radius, Sides, Colour, Style, Index, Flags, RFlags>, const auto& params) {
  return make_ngon(fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
                   cvec4{evaluate(Colour{}, params)}, ngon_style{evaluate(Style{}, params)},
                   static_cast<unsigned char>(evaluate(Index{}, params)),
                   shape_flag{evaluate(Flags{}, params)}, render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, ngon_style Style = ngon_style::kPolygon,
          unsigned char Index = 0, shape_flag Flags = shape_flag::kNone,
          render::flag RFlags = render::flag::kNone>
using ngon = constant<make_ngon(Radius, Sides, Colour, Style, Index, Flags, RFlags)>;

template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polygon = ngon<Radius, Sides, Colour, ngon_style::kPolygon, Index, Flags, RFlags>;
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polystar = ngon<Radius, Sides, Colour, ngon_style::kPolystar, Index, Flags, RFlags>;
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polygram = ngon<Radius, Sides, Colour, ngon_style::kPolygram, Index, Flags, RFlags>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          ngon_style Style = ngon_style::kPolygon, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using ngon_colour_p =
    ngon_eval<constant<Radius>, constant<Sides>, parameter<ParameterIndex>, constant<Style>,
              constant<Index>, constant<Flags>, constant<RFlags>>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polygon_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygon, Index, Flags, RFlags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polystar_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolystar, Index, Flags, RFlags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using polygram_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygram, Index, Flags, RFlags>;

}  // namespace ii::geom::legacy

#endif