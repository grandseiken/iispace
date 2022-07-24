#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_POLYARC_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_POLYARC_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {

struct polyarc_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = 0;
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr shape_flag check_point_legacy(const vec2& v, shape_flag mask) const {
    if (!(flags & mask)) {
      return shape_flag::kNone;
    }
    auto theta = angle(v);
    auto r = length(v);
    return 0 <= theta && theta <= (2 * fixed_c::pi * segments) / sides && r >= radius - 10 &&
            r < radius
        ? flags & mask
        : shape_flag::kNone;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) const {
    for (std::uint32_t i = 0; sides >= 2 && i < sides && i < segments; ++i) {
      auto a = from_polar(i * 2 * fixed_c::pi / sides, radius);
      auto b = from_polar((i + 1) * 2 * fixed_c::pi / sides, radius);
      std::invoke(f, t.translate(a).v, t.translate(b).v, colour);
    }
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) const {
    std::invoke(f, t.v, colour);
  }
};

constexpr polyarc_data make_polyarc(fixed radius, std::uint32_t sides, std::uint32_t segments,
                                    const glm::vec4& colour, shape_flag flags = shape_flag::kNone) {
  return {{}, radius, sides, segments, colour, flags};
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides,
          Expression<std::uint32_t> Segments, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct polyarc_eval {};

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides,
          Expression<std::uint32_t> Segments, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags>
constexpr auto evaluate(polyarc_eval<Radius, Sides, Segments, Colour, Flags>, const auto& params) {
  return make_polyarc(fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
                      std::uint32_t{evaluate(Segments{}, params)},
                      glm::vec4{evaluate(Colour{}, params)}, shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed Radius, std::uint32_t Sides, std::uint32_t Segments, glm::vec4 Colour,
          shape_flag Flags = shape_flag::kNone>
using polyarc = constant<make_polyarc(Radius, Sides, Segments, Colour, Flags)>;

template <fixed Radius, std::uint32_t Sides, std::uint32_t Segments, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polyarc_colour_p = polyarc_eval<constant<Radius>, constant<Sides>, constant<Segments>,
                                      parameter<ParameterIndex>, constant<Flags>>;

}  // namespace ii::geom

#endif