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

  constexpr void iterate(iterate_flags_t, const null_transform&, FlagFunction auto&& f) const {
    std::invoke(f, flags);
  }

  void
  iterate(iterate_check_collision_t it, const convert_local_transform& t, hit_result& hit) const;
};

constexpr box_collider_data
make_box_collider(const vec2& dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<vec2> Dimensions, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_collider_eval : shape_node {};

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

  constexpr void iterate(iterate_lines_t, const Transform auto& t, LineFunction auto&& f) const {
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

  constexpr void iterate(iterate_shapes_t, const Transform auto& t, ShapeFunction auto&& f) const {
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
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t, std::min(dimensions.x, dimensions.y), line.colour0, fill.colour0);
  }
};

constexpr box_data make_box(const vec2& dimensions, line_style line, fill_style fill = sfill(),
                            render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<vec2>, Expression<line_style>, Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct box_eval : shape_node {};

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