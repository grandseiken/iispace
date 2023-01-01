#ifndef II_GAME_GEOMETRY_SHAPES_BOX_H
#define II_GAME_GEOMETRY_SHAPES_BOX_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace shapes {

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct box_collider_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_check_point_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (+(flags & it.mask) && abs((*t).x) <= dimensions.x && abs((*t).y) <= dimensions.y) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void
  iterate(iterate_check_line_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (+(flags & it.mask) && intersect_aabb_line(-dimensions, dimensions, t.get_a(), t.get_b())) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void
  iterate(iterate_check_ball_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (+(flags & it.mask) && intersect_aabb_ball(-dimensions, dimensions, *t, t.r)) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void
  iterate(iterate_check_convex_t it, const Transform auto& t, const HitFunction auto& f) const {
    if (!+(flags & it.mask)) {
      return;
    }
    auto va = *t;
    if (intersect_aabb_convex(-dimensions, dimensions, va)) {
      std::invoke(f, flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }
};

constexpr box_collider_data
make_box_collider(const vec2& dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<vec2> Dimensions, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_collider_eval {};

template <Expression<vec2> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(box_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_box_collider(vec2{evaluate(Dimensions{}, params)},
                           shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct box_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  line_style line;
  fill_style fill;
  render::flag flags = render::flag::kNone;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    if (!line.colour0.a && !line.colour1.a) {
      return;
    }
    auto a = *t.translate({dimensions.x, dimensions.y});
    auto b = *t.translate({-dimensions.x, dimensions.y});
    auto c = *t.translate({-dimensions.x, -dimensions.y});
    auto d = *t.translate({dimensions.x, -dimensions.y});

    // TODO: need line gradients to match rendering, if we use them.
    std::invoke(f, a, b, line.colour0, line.width, line.z);
    std::invoke(f, b, c, line.colour0, line.width, line.z);
    std::invoke(f, c, d, line.colour1, line.width, line.z);
    std::invoke(f, d, a, line.colour1, line.width, line.z);
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    if (line.colour0.a || line.colour1.a) {
      std::invoke(
          f,
          render::shape{
              .origin = to_float(*t),
              .rotation = t.rotation().to_float(),
              .colour0 = line.colour0,
              .colour1 = line.colour1,
              .z_index = line.z,
              .s_index = line.index,
              .flags = flags,
              .data = render::box{.dimensions = to_float(dimensions), .line_width = line.width},
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
                      .data = render::box_fill{.dimensions = to_float(dimensions)},
                  });
    }
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line.colour0.a ? line.colour0 : fill.colour0);
  }
};

constexpr box_data make_box(const vec2& dimensions, line_style line, fill_style fill = sfill(),
                            render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<vec2>, Expression<line_style>, Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct box_eval {};

template <Expression<vec2> Dimensions, Expression<line_style> Line, Expression<fill_style> Fill,
          Expression<render::flag> RFlags>
constexpr auto evaluate(box_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, evaluate(Line{}, params),
                  evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 Dimensions, shape_flag Flags = shape_flag::kNone>
using box_collider = constant<make_box_collider(Dimensions, Flags)>;
template <vec2 Dimensions, line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using box = constant<make_box(Dimensions, Line, Fill, RFlags)>;
template <vec2 Dimensions, line_style Line, fill_style Fill, shape_flag Flags = shape_flag::kNone,
          render::flag RFlags = render::flag::kNone>
using box_with_collider =
    compound<box<Dimensions, Line, Fill, RFlags>, box_collider<Dimensions, Flags>>;

template <vec2 Dimensions, std::size_t N, line_style Line = sline(),
          render::flag RFlags = render::flag::kNone>
using box_colour_p =
    box_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <vec2 Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill(), render::flag RFlags = render::flag::kNone>
using box_colour_p2 = box_eval<constant<Dimensions>, set_colour_p<Line, N0>, set_colour_p<Fill, N1>,
                               constant<RFlags>>;

}  // namespace shapes
}  // namespace ii::geom

#endif