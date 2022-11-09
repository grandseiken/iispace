#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_NGON_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/node.h"
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

struct ngon_line_style {
  ngon_style style = ngon_style::kPolygon;
  glm::vec4 colour{0.f};
  float width = 1.f;
};

struct ngon_fill_style {
  glm::vec4 colour{0.f};
};

constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, sides, segments ? segments : sides};
}

constexpr ngon_line_style nline(const glm::vec4& colour = glm::vec4{0.f}, float width = 1.f) {
  return {.colour = colour, .width = width};
}

constexpr ngon_line_style nline(ngon_style style, const glm::vec4& colour, float width = 1.f) {
  return {.style = style, .colour = colour, .width = width};
}

constexpr ngon_fill_style nfill(const glm::vec4& colour = glm::vec4{0.f}) {
  return {colour};
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
  ngon_line_style line_style;
  ngon_fill_style fill_style;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    auto vertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * fixed_c::pi / dimensions.sides).translate({dimensions.radius, 0});
    };

    if (line_style.style != ngon_style::kPolygram) {
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        std::invoke(f, vertex(i), line_style.style == ngon_style::kPolygon ? vertex(i + 1) : t.v,
                    line_style.colour);
      }
      return;
    }
    for (std::size_t i = 0; i < dimensions.segments; ++i) {
      for (std::size_t j = i + 1; j < dimensions.segments; ++j) {
        std::invoke(f, vertex(i), vertex(j), line_style.colour);
      }
    }
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    if (line_style.colour.a && dimensions.sides == dimensions.segments) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour = line_style.colour,
                      .data = render::ngon{.radius = dimensions.radius.to_float(),
                                           .sides = dimensions.sides,
                                           .style = line_style.style,
                                           .line_width = line_style.width},
                  });
    } else if (line_style.colour.a) {
      // TODO: handle line_style.style.
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour = line_style.colour,
                      .data = render::polyarc{.radius = dimensions.radius.to_float(),
                                              .sides = dimensions.sides,
                                              .segments = dimensions.segments,
                                              .line_width = line_style.width},
                  });
    }
    // TODO: fill_style.
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line_style.colour.a ? line_style.colour : fill_style.colour);
  }
};

constexpr ngon_data
make_ngon(ngon_dimensions dimensions, ngon_line_style line, ngon_fill_style fill) {
  return {{}, dimensions, line, fill};
}

template <Expression<ngon_dimensions>, Expression<ngon_line_style>,
          Expression<ngon_fill_style> = constant<nfill()>>
struct ngon_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<ngon_line_style> Line,
          Expression<ngon_fill_style> Fill = constant<nfill()>>
constexpr auto evaluate(ngon_eval<Dimensions, Line, Fill>, const auto& params) {
  return make_ngon(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ngon_dimensions Dimensions, shape_flag Flags = shape_flag::kNone>
using ngon_collider = constant<make_ngon_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, ngon_fill_style Fill = nfill()>
using ngon = constant<make_ngon(Dimensions, Line, Fill)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, ngon_fill_style Fill,
          shape_flag Flags = shape_flag::kNone>
using ngon_with_collider = compound<ngon<Dimensions, Line, Fill>, ngon_collider<Dimensions, Flags>>;

template <ngon_dimensions Dimensions, std::size_t N, ngon_line_style Line = nline()>
using ngon_colour_p = ngon_eval<constant<Dimensions>, set_colour_p<Line, N>>;

}  // namespace shapes
}  // namespace ii::geom

#endif