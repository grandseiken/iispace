#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/logic/sim/io/render.h"
#include <glm/glm.hpp>
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
  iterate(iterate_collision_t it, const Transform auto& t, const FlagFunction auto& f) const {
    if (+(flags & it.mask) && abs((*t).x) < dimensions.x && abs((*t).y) < dimensions.y) {
      std::invoke(f, flags & it.mask);
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

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    if (!line.colour.a) {
      return;
    }
    auto a = *t.translate({dimensions.x, dimensions.y});
    auto b = *t.translate({-dimensions.x, dimensions.y});
    auto c = *t.translate({-dimensions.x, -dimensions.y});
    auto d = *t.translate({dimensions.x, -dimensions.y});

    std::invoke(f, a, b, line.colour, line.width, line.z);
    std::invoke(f, b, c, line.colour, line.width, line.z);
    std::invoke(f, c, d, line.colour, line.width, line.z);
    std::invoke(f, d, a, line.colour, line.width, line.z);
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    if (line.colour.a) {
      std::invoke(
          f,
          render::shape{
              .origin = to_float(*t),
              .rotation = t.rotation().to_float(),
              .colour = line.colour,
              .z_index = line.z,
              .s_index = line.index,
              .data = render::box{.dimensions = to_float(dimensions), .line_width = line.width},
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
                      .data = render::box_fill{.dimensions = to_float(dimensions)},
                  });
    }
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line.colour.a ? line.colour : fill.colour);
  }
};

constexpr box_data make_box(const vec2& dimensions, line_style line, fill_style fill = sfill()) {
  return {{}, dimensions, line, fill};
}

template <Expression<vec2> Dimensions, Expression<line_style> Line,
          Expression<fill_style> Fill = constant<sfill()>>
struct box_eval {};

template <Expression<vec2> Dimensions, Expression<line_style> Line, Expression<fill_style> Fill>
constexpr auto evaluate(box_eval<Dimensions, Line, Fill>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, evaluate(Line{}, params),
                  evaluate(Fill{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 Dimensions, shape_flag Flags = shape_flag::kNone>
using box_collider = constant<make_box_collider(Dimensions, Flags)>;
template <vec2 Dimensions, line_style Line, fill_style Fill = sfill()>
using box = constant<make_box(Dimensions, Line, Fill)>;
template <vec2 Dimensions, line_style Line, fill_style Fill, shape_flag Flags = shape_flag::kNone>
using box_with_collider = compound<box<Dimensions, Line, Fill>, box_collider<Dimensions, Flags>>;

template <vec2 Dimensions, std::size_t N, line_style Line = sline()>
using box_colour_p = box_eval<constant<Dimensions>, set_colour_p<Line, N>>;
template <vec2 Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill()>
using box_colour_p2 =
    box_eval<constant<Dimensions>, set_colour_p<Line, N0>, set_colour_p<Fill, N1>>;

}  // namespace shapes
}  // namespace ii::geom

#endif