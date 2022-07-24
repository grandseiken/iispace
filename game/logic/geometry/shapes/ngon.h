#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {

enum class ngon_style {
  kPolygon,
  kPolystar,
  kPolygram,
};

struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  glm::vec4 colour{0.f};
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;

  constexpr shape_flag check_point_legacy(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius * radius ? flags & mask
                                                                      : shape_flag::kNone;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) const {
    auto vertex = [&](std::uint32_t i) {
      return t.rotate(i * 2 * fixed_c::pi / sides).translate({radius, 0}).v;
    };

    if (style != ngon_style::kPolygram) {
      for (std::uint32_t i = 0; i < sides; ++i) {
        std::invoke(f, vertex(i), style == ngon_style::kPolygon ? vertex(i + 1) : t.v, colour);
      }
      return;
    }
    for (std::size_t i = 0; i < sides; ++i) {
      for (std::size_t j = i + 1; j < sides; ++j) {
        std::invoke(f, vertex(i), vertex(j), colour);
      }
    }
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) const {
    std::invoke(f, t.v, colour);
  }
};

constexpr ngon_data make_ngon(fixed radius, std::uint32_t sides, const glm::vec4& colour,
                              ngon_style style = ngon_style::kPolygon,
                              shape_flag flags = shape_flag::kNone) {
  return {{}, radius, sides, colour, style, flags};
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<glm::vec4> Colour,
          Expression<ngon_style> Style = constant<ngon_style::kPolygon>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_eval {};

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<glm::vec4> Colour,
          Expression<ngon_style> Style, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_eval<Radius, Sides, Colour, Style, Flags>, const auto& params) {
  return make_ngon(fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
                   glm::vec4{evaluate(Colour{}, params)}, ngon_style{evaluate(Style{}, params)},
                   shape_flag{evaluate(Flags{}, params)});
}

template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon = constant<make_ngon(Radius, Sides, Colour, Style, Flags)>;

template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polygon = ngon<Radius, Sides, Colour, ngon_style::kPolygon, Flags>;
template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polystar = ngon<Radius, Sides, Colour, ngon_style::kPolystar, Flags>;
template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polygram = ngon<Radius, Sides, Colour, ngon_style::kPolygram, Flags>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon_colour_p = ngon_eval<constant<Radius>, constant<Sides>, parameter<ParameterIndex>,
                                constant<Style>, constant<Flags>>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygon_colour_p = ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygon, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polystar_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolystar, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygram_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygram, Flags>;

}  // namespace ii::geom

#endif