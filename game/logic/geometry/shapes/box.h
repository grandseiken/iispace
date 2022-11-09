#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/node.h"
#include "game/logic/sim/io/render.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace shapes {

struct box_line_style {
  glm::vec4 colour{0.f};
  float width = 1.f;
};

struct box_fill_style {
  glm::vec4 colour{0.f};
};

constexpr box_line_style bline(const glm::vec4& colour = glm::vec4{0.f}, float width = 1.f) {
  return {.colour = colour, .width = width};
}

constexpr box_fill_style bfill(const glm::vec4& colour = glm::vec4{0.f}) {
  return {colour};
}

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
  box_line_style line_style;
  box_fill_style fill_style;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    auto a = *t.translate({dimensions.x, dimensions.y});
    auto b = *t.translate({-dimensions.x, dimensions.y});
    auto c = *t.translate({-dimensions.x, -dimensions.y});
    auto d = *t.translate({dimensions.x, -dimensions.y});

    std::invoke(f, a, b, line_style.colour);
    std::invoke(f, b, c, line_style.colour);
    std::invoke(f, c, d, line_style.colour);
    std::invoke(f, d, a, line_style.colour);
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    if (line_style.colour.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour = line_style.colour,
                      .data = render::box{.dimensions = to_float(dimensions),
                                          .line_width = line_style.width},
                  });
    }
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t, line_style.colour.a ? line_style.colour : fill_style.colour);
  }
};

constexpr box_data make_box(const vec2& dimensions, box_line_style line, box_fill_style fill) {
  return {{}, dimensions, line, fill};
}

template <Expression<vec2> Dimensions, Expression<box_line_style> Line,
          Expression<box_fill_style> Fill = constant<bfill()>>
struct box_eval {};

template <Expression<vec2> Dimensions, Expression<box_line_style> Line,
          Expression<box_fill_style> Fill>
constexpr auto evaluate(box_eval<Dimensions, Line, Fill>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, evaluate(Line{}, params),
                  evaluate(Fill{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 Dimensions, shape_flag Flags = shape_flag::kNone>
using box_collider = constant<make_box_collider(Dimensions, Flags)>;
template <vec2 Dimensions, box_line_style Line, box_fill_style Fill = bfill()>
using box = constant<make_box(Dimensions, Line, Fill)>;
template <vec2 Dimensions, box_line_style Line, box_fill_style Fill,
          shape_flag Flags = shape_flag::kNone>
using box_with_collider = compound<box<Dimensions, Line, Fill>, box_collider<Dimensions, Flags>>;

template <vec2 Dimensions, std::size_t N, box_line_style Line = bline()>
using box_colour_p = box_eval<constant<Dimensions>, set_colour_p<Line, N>>;

}  // namespace shapes
}  // namespace ii::geom

#endif