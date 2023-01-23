#ifndef II_GAME_GEOMETRY_SHAPES_BALL_H
#define II_GAME_GEOMETRY_SHAPES_BALL_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"

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

  constexpr void iterate(iterate_flags_t, const null_transform&, FlagFunction auto&& f) const {
    std::invoke(f, flags);
  }

  void
  iterate(iterate_check_collision_t it, const convert_local_transform& t, hit_result& hit) const;
};

constexpr ball_collider_data
make_ball_collider(ball_dimensions dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ball_dimensions> Dimensions, Expression<shape_flag> Flags>
struct ball_collider_eval : shape_node {};

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
  render::flag flags = render::flag::kNone;

  constexpr void iterate(iterate_lines_t, const Transform auto& t, LineFunction auto&& f) const {
    if (!line.colour0.a) {
      return;
    }
    std::uint32_t n = std::max(3, dimensions.radius.to_int() / 2);
    std::uint32_t in = std::max(3, dimensions.inner_radius.to_int() / 2);
    auto vertex = [&](fixed r, std::uint32_t i, std::uint32_t n) {
      return *t.rotate(i * 2 * pi<fixed> / n).translate({r, 0});
    };

    for (std::uint32_t i = 0; i < n; ++i) {
      std::invoke(f, vertex(dimensions.radius, i, n), vertex(dimensions.radius, i + 1, n),
                  line.colour0, line.width, line.z);
    }
    for (std::uint32_t i = 0; dimensions.inner_radius && i < in; ++i) {
      std::invoke(f, vertex(dimensions.inner_radius, i, in),
                  vertex(dimensions.inner_radius, i + 1, in), line.colour0, line.width, line.z);
    }
  }

  constexpr void iterate(iterate_shapes_t, const Transform auto& t, ShapeFunction auto&& f) const {
    if (line.colour0.a || line.colour1.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour0 = line.colour0,
                      .colour1 = line.colour1,
                      .z_index = line.z,
                      .s_index = line.index,
                      .flags = flags,
                      .data = render::ball{.radius = dimensions.radius.to_float(),
                                           .inner_radius = dimensions.inner_radius.to_float(),
                                           .line_width = line.width},
                  });
    }
    if (fill.colour0.a || fill.colour1.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour0 = fill.colour0,
                      .colour1 = fill.colour1,
                      .z_index = fill.z,
                      .s_index = fill.index,
                      .flags = flags,
                      .data = render::ball_fill{.radius = dimensions.radius.to_float(),
                                                .inner_radius = dimensions.inner_radius.to_float()},
                  });
    }
  }

  constexpr void
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t, dimensions.radius, line.colour0, fill.colour0);
  }
};

constexpr ball_data make_ball(ball_dimensions dimensions, line_style line,
                              fill_style fill = sfill(), render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<ball_dimensions>, Expression<line_style>,
          Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct ball_eval : shape_node {};

template <Expression<ball_dimensions> Dimensions, Expression<line_style> Line,
          Expression<fill_style> Fill, Expression<render::flag> RFlags>
constexpr auto evaluate(ball_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_ball(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ball_dimensions Dimensions, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Dimensions, Flags)>;
template <ball_dimensions Dimensions, line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ball = constant<make_ball(Dimensions, Line, Fill, RFlags)>;
template <ball_dimensions Dimensions, line_style Line, fill_style Fill,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using ball_with_collider =
    compound<ball<Dimensions, Line, Fill, RFlags>, ball_collider<Dimensions, Flags>>;

template <ball_dimensions Dimensions, std::size_t N, line_style Line = sline(),
          render::flag RFlags = render::flag::kNone>
using ball_colour_p =
    ball_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <ball_dimensions Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill(), render::flag RFlags = render::flag::kNone>
using ball_colour_p2 = ball_eval<constant<Dimensions>, set_colour_p<Line, N0>,
                                 set_colour_p<Fill, N1>, constant<RFlags>>;

}  // namespace shapes
}  // namespace ii::geom

#endif