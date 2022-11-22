#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/data.h"
#include "game/logic/sim/io/render.h"

namespace ii::geom {
inline namespace shapes {

struct ball_dimensions {
  fixed radius = 0;
  fixed inner_radius = 0;
};

constexpr ball_dimensions bd(fixed radius = 0, fixed inner_radius = 0) {
  return {radius, inner_radius};
}

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ball_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_collision_t it, const Transform auto& t, const FlagFunction auto& f) const {
    auto d_sq = length_squared(t.deref_ignore_rotation());
    if (+(flags & it.mask) && d_sq <= dimensions.radius * dimensions.radius &&
        d_sq >= dimensions.inner_radius * dimensions.inner_radius) {
      std::invoke(f, flags & it.mask);
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }
};

constexpr ball_collider_data
make_ball_collider(ball_dimensions dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ball_dimensions> Dimensions, Expression<shape_flag> Flags>
struct ball_collider_eval {};

template <Expression<fixed> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(ball_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_ball_collider(evaluate(Dimensions{}, params), shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct ball_data : shape_data_base {
  using shape_data_base::iterate;
  ball_dimensions dimensions;
  line_style line;
  fill_style fill;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    if (!line.colour.a) {
      return;
    }
    std::uint32_t n = dimensions.radius.to_int();
    std::uint32_t in = dimensions.inner_radius.to_int();
    auto vertex = [&](fixed r, std::uint32_t i, std::uint32_t n) {
      return *t.rotate(i * 2 * fixed_c::pi / n).translate({r, 0});
    };

    for (std::uint32_t i = 0; i < n; ++i) {
      std::invoke(f, vertex(dimensions.radius, i, n), vertex(dimensions.radius, i + 1, n),
                  line.colour, line.width, line.z);
    }
    for (std::uint32_t i = 0; dimensions.inner_radius && i < in; ++i) {
      std::invoke(f, vertex(dimensions.inner_radius, i, in),
                  vertex(dimensions.inner_radius, i + 1, in), line.colour, line.width, line.z);
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
                      .data = render::ball{.radius = dimensions.radius.to_float(),
                                           .inner_radius = dimensions.inner_radius.to_float(),
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
                      .data = render::ball_fill{.radius = dimensions.radius.to_float(),
                                                .inner_radius = dimensions.inner_radius.to_float()},
                  });
    }
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line.colour.a ? line.colour : fill.colour);
  }
};

constexpr ball_data
make_ball(ball_dimensions dimensions, line_style line, fill_style fill = sfill()) {
  return {{}, dimensions, line, fill};
}

template <Expression<ball_dimensions>, Expression<line_style>,
          Expression<fill_style> = constant<sfill()>>
struct ball_eval {};

template <Expression<ball_dimensions> Dimensions, Expression<line_style> Line,
          Expression<fill_style> Fill>
constexpr auto evaluate(ball_eval<Dimensions, Line, Fill>, const auto& params) {
  return make_ball(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ball_dimensions Dimensions, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Dimensions, Flags)>;
template <ball_dimensions Dimensions, line_style Line, fill_style Fill = sfill()>
using ball = constant<make_ball(Dimensions, Line, Fill)>;
template <ball_dimensions Dimensions, line_style Line, fill_style Fill,
          shape_flag Flags = shape_flag::kNone>
using ball_with_collider = compound<ball<Dimensions, Line, Fill>, ball_collider<Dimensions, Flags>>;

template <ball_dimensions Dimensions, std::size_t N, line_style Line = sline()>
using ball_colour_p = ball_eval<constant<Dimensions>, set_colour_p<Line, N>>;
template <ball_dimensions Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill()>
using ball_colour_p2 =
    ball_eval<constant<Dimensions>, set_colour_p<Line, N0>, set_colour_p<Fill, N1>>;

}  // namespace shapes
}  // namespace ii::geom

#endif