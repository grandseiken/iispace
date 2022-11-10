#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/node.h"
#include "game/logic/geometry/shapes/data.h"
#include "game/logic/sim/io/render.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace shapes {
using render::ngon_style;

struct ngon_dimensions {
  fixed radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct ngon_line_style : line_style {
  ngon_style style = ngon_style::kPolygon;
};

constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, sides, segments ? segments : sides};
}

constexpr ngon_line_style
nline(const glm::vec4& colour = glm::vec4{0.f}, float z = 0.f, float width = 1.f) {
  return {{.colour = colour, .z = z, .width = width}};
}

constexpr ngon_line_style
nline(ngon_style style, const glm::vec4& colour, float z = 0.f, float width = 1.f) {
  return {{.colour = colour, .z = z, .width = width}, style};
}

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_collision_t it, const Transform auto& t, const FlagFunction auto& f) const {
    if (!(flags & it.mask)) {
      return;
    }
    auto v = *t;
    auto theta = angle(v);
    if (theta < 0) {
      theta += 2 * fixed_c::pi;
    }
    auto a = 2 * fixed_c::pi / dimensions.sides;
    auto at = (theta % a) / a;
    auto d_sq = dimensions.radius * dimensions.radius * (1 + 2 * at * (at - 1) * (1 - cos(a)));
    if (theta <= a * dimensions.segments && length_squared(v) <= d_sq) {
      std::invoke(f, flags & it.mask);
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }
};

constexpr ngon_collider_data
make_ngon_collider(ngon_dimensions dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ngon_dimensions>, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_collider_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_ngon_collider(evaluate(Dimensions{}, params), shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  ngon_line_style line;
  fill_style fill;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    auto vertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * fixed_c::pi / dimensions.sides).translate({dimensions.radius, 0});
    };

    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      std::invoke(f, vertex(i), line.style != ngon_style::kPolystar ? vertex(i + 1) : t.v,
                  line.colour, line.width);
    }
    if (line.style == ngon_style::kPolygram) {
      for (std::size_t i = 0; i < dimensions.sides; ++i) {
        for (std::size_t j = i + 2; j < dimensions.sides && (j + 1) % dimensions.sides != i; ++j) {
          std::invoke(f, vertex(i), vertex(j), line.colour, line.width);
        }
      }
    }
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    if (line.colour.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour = line.colour,
                      .data = render::ngon{.radius = dimensions.radius.to_float(),
                                           .sides = dimensions.sides,
                                           .segments = dimensions.segments,
                                           .style = line.style,
                                           .line_width = line.width},
                  });
    }
    // TODO: fill.
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line.colour.a ? line.colour : fill.colour);
  }
};

constexpr ngon_data
make_ngon(ngon_dimensions dimensions, ngon_line_style line, fill_style fill = sfill()) {
  return {{}, dimensions, line, fill};
}

template <Expression<ngon_dimensions>, Expression<ngon_line_style>,
          Expression<fill_style> = constant<sfill()>>
struct ngon_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<ngon_line_style> Line,
          Expression<fill_style> Fill>
constexpr auto evaluate(ngon_eval<Dimensions, Line, Fill>, const auto& params) {
  return make_ngon(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ngon_dimensions Dimensions, shape_flag Flags = shape_flag::kNone>
using ngon_collider = constant<make_ngon_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill = sfill()>
using ngon = constant<make_ngon(Dimensions, Line, Fill)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill,
          shape_flag Flags = shape_flag::kNone>
using ngon_with_collider = compound<ngon<Dimensions, Line, Fill>, ngon_collider<Dimensions, Flags>>;

template <ngon_dimensions Dimensions, std::size_t N, ngon_line_style Line = nline()>
using ngon_colour_p = ngon_eval<constant<Dimensions>, set_colour_p<Line, N>>;

}  // namespace shapes
}  // namespace ii::geom

#endif