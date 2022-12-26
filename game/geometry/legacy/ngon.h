#ifndef II_GAME_GEOMETRY_LEGACY_NGON_H
#define II_GAME_GEOMETRY_LEGACY_NGON_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace legacy {
using render::ngon_style;

struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  cvec4 colour{0.f};
  unsigned char index = 0;
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_check_point_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (+(flags & it.mask) && length_squared(t.deref_ignore_rotation()) < radius * radius) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    auto vertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * pi<fixed> / sides).translate({radius, 0});
    };

    if (style != ngon_style::kPolygram) {
      for (std::uint32_t i = 0; i < sides; ++i) {
        std::invoke(f, vertex(i), style == ngon_style::kPolygon ? vertex(i + 1) : t.v, colour, 1.f,
                    0.f);
      }
      return;
    }
    for (std::size_t i = 0; i < sides; ++i) {
      for (std::size_t j = i + 1; j < sides; ++j) {
        std::invoke(f, vertex(i), vertex(j), colour, 1.f, 0.f);
      }
    }
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    std::invoke(
        f,
        render::shape{
            .origin = to_float(*t),
            .rotation = t.rotation().to_float(),
            .colour = colour,
            .s_index = index,
            .data = render::ngon{.radius = radius.to_float(), .sides = sides, .style = style},
        });
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, colour);
  }
};

constexpr ngon_data make_ngon(fixed radius, std::uint32_t sides, const cvec4& colour,
                              ngon_style style = ngon_style::kPolygon, unsigned char index = 0,
                              shape_flag flags = shape_flag::kNone) {
  return {{}, radius, sides, colour, index, style, flags};
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<cvec4> Colour,
          Expression<ngon_style> Style = constant<ngon_style::kPolygon>,
          Expression<unsigned char> Index = constant<0>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_eval {};

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<cvec4> Colour,
          Expression<ngon_style> Style, Expression<unsigned char> Index,
          Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_eval<Radius, Sides, Colour, Style, Index, Flags>, const auto& params) {
  return make_ngon(fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
                   cvec4{evaluate(Colour{}, params)}, ngon_style{evaluate(Style{}, params)},
                   static_cast<unsigned char>(evaluate(Index{}, params)),
                   shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, ngon_style Style = ngon_style::kPolygon,
          unsigned char Index = 0, shape_flag Flags = shape_flag::kNone>
using ngon = constant<make_ngon(Radius, Sides, Colour, Style, Index, Flags)>;

template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polygon = ngon<Radius, Sides, Colour, ngon_style::kPolygon, Index, Flags>;
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polystar = ngon<Radius, Sides, Colour, ngon_style::kPolystar, Index, Flags>;
template <fixed Radius, std::uint32_t Sides, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polygram = ngon<Radius, Sides, Colour, ngon_style::kPolygram, Index, Flags>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          ngon_style Style = ngon_style::kPolygon, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using ngon_colour_p = ngon_eval<constant<Radius>, constant<Sides>, parameter<ParameterIndex>,
                                constant<Style>, constant<Index>, constant<Flags>>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polygon_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygon, Index, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polystar_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolystar, Index, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using polygram_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygram, Index, Flags>;

}  // namespace legacy
}  // namespace ii::geom

#endif