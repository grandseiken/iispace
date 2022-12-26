#ifndef II_GAME_GEOMETRY_SHAPES_NGON_H
#define II_GAME_GEOMETRY_SHAPES_NGON_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace shapes {
using render::ngon_style;

struct ngon_dimensions {
  fixed radius = 0;
  fixed inner_radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct ngon_line_style : line_style {
  ngon_style style = ngon_style::kPolygon;
};

constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, 0, sides, segments ? segments : sides};
}

constexpr ngon_dimensions
nd2(fixed radius, fixed inner_radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, inner_radius, sides, segments ? segments : sides};
}

constexpr ngon_line_style
nline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {{.colour = colour, .z = z, .width = width, .index = index}};
}

constexpr ngon_line_style nline(ngon_style style, const cvec4& colour, float z = 0.f,
                                float width = 1.f, unsigned char index = 0) {
  return {{.colour = colour, .z = z, .width = width, .index = index}, style};
}

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_check_point_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (!(flags & it.mask)) {
      return;
    }
    auto v = *t;
    auto theta = angle(v);
    if (theta < 0) {
      theta += 2 * pi<fixed>;
    }
    auto a = 2 * pi<fixed> / dimensions.sides;
    auto at = (theta % a) / a;
    auto dt = (1 + 2 * at * (at - 1) * (1 - cos(a)));
    auto d_sq = dt * dimensions.radius * dimensions.radius;
    auto i_sq = dt * dimensions.inner_radius * dimensions.inner_radius;
    if (theta <= a * dimensions.segments && length_squared(v) <= d_sq &&
        length_squared(v) >= i_sq) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void
  iterate(iterate_check_line_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (!(flags & it.mask)) {
      return;
    }
    std::array<vec2, 4> va{};
    auto line_a = t.get_a();
    auto line_b = t.get_b();
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = convex_segment(va, i);
      if (intersect_convex_line(convex, line_a, line_b)) {
        std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  }

  constexpr void
  iterate(iterate_check_ball_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (!(flags & it.mask)) {
      return;
    }
    std::array<vec2, 4> va{};
    auto c = *t;
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = convex_segment(va, i);
      if (intersect_convex_ball(convex, c, t.r)) {
        std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  }

  constexpr void
  iterate(iterate_check_convex_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (!(flags & it.mask)) {
      return;
    }
    auto va0 = *t;
    std::array<vec2, 4> va1{};
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = convex_segment(va1, i);
      if (intersect_convex_convex(convex, va0)) {
        std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr std::span<const vec2> convex_segment(std::array<vec2, 4>& va, std::uint32_t i) const {
    if (dimensions.inner_radius) {
      vec2 a = ::rotate(vec2{1, 0}, 2 * i * pi<fixed> / dimensions.sides);
      vec2 b = ::rotate(vec2{1, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
      va[0] = dimensions.inner_radius * a;
      va[1] = dimensions.inner_radius * b;
      va[2] = dimensions.radius * b;
      va[3] = dimensions.radius * a;
      return {va.begin(), va.begin() + 4};
    }
    va[0] = vec2{0};
    va[1] = ::rotate(vec2{dimensions.radius, 0}, 2 * i * pi<fixed> / dimensions.sides);
    va[2] = ::rotate(vec2{dimensions.radius, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
    return {va.begin(), va.begin() + 3};
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
    if (!line.colour.a) {
      return;
    }
    auto vertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * pi<fixed> / dimensions.sides).translate({dimensions.radius, 0});
    };
    auto ivertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * pi<fixed> / dimensions.sides)
                  .translate({dimensions.inner_radius, 0});
    };

    switch (line.style) {
    case ngon_style::kPolystar:
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        std::invoke(f, ivertex(i), vertex(i), line.colour, line.width, line.z);
      }
      break;
    case ngon_style::kPolygram:
      for (std::size_t i = 0; i < dimensions.sides; ++i) {
        for (std::size_t j = i + 2; j < dimensions.sides && (j + 1) % dimensions.sides != i; ++j) {
          std::invoke(f, vertex(i), vertex(j), line.colour, line.width, line.z);
        }
      }
      // Fallthrough.
    case ngon_style::kPolygon:
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        std::invoke(f, vertex(i), vertex(i + 1), line.colour, line.width, line.z);
      }
      for (std::uint32_t i = 0; dimensions.inner_radius && i < dimensions.segments; ++i) {
        std::invoke(f, ivertex(i), ivertex(i + 1), line.colour, line.width, line.z);
      }
      break;
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
                      .z_index = line.z,
                      .s_index = line.index,
                      .data = render::ngon{.radius = dimensions.radius.to_float(),
                                           .inner_radius = dimensions.inner_radius.to_float(),
                                           .sides = dimensions.sides,
                                           .segments = dimensions.segments,
                                           .style = line.style,
                                           .line_width = line.width},
                  });
    }
    if (fill.colour.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour = fill.colour,
                      .z_index = fill.z,
                      .s_index = fill.index,
                      .data = render::ngon_fill{.radius = dimensions.radius.to_float(),
                                                .inner_radius = dimensions.inner_radius.to_float(),
                                                .sides = dimensions.sides,
                                                .segments = dimensions.segments},
                  });
    }
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
template <ngon_dimensions Dimensions, std::size_t N0, std::size_t N1,
          ngon_line_style Line = nline(), fill_style Fill = sfill()>
using ngon_colour_p2 =
    ngon_eval<constant<Dimensions>, set_colour_p<Line, N0>, set_colour_p<Fill, N1>>;

}  // namespace shapes
}  // namespace ii::geom

#endif