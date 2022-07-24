#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BOX_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {

struct box_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr shape_flag check_point_legacy(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && abs(v.x) < dimensions.x && abs(v.y) < dimensions.y
        ? flags & mask
        : shape_flag::kNone;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) const {
    auto a = t.translate({dimensions.x, dimensions.y}).v;
    auto b = t.translate({-dimensions.x, dimensions.y}).v;
    auto c = t.translate({-dimensions.x, -dimensions.y}).v;
    auto d = t.translate({dimensions.x, -dimensions.y}).v;

    std::invoke(f, a, b, colour);
    std::invoke(f, b, c, colour);
    std::invoke(f, c, d, colour);
    std::invoke(f, d, a, colour);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) const {
    std::invoke(f, t.v, colour);
  }
};

constexpr box_data
make_box(const vec2& dimensions, const glm::vec4& colour, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, colour, flags};
}

template <Expression<vec2> Dimensions, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_eval {};

template <Expression<vec2> Dimensions, Expression<glm::vec4> Colour, Expression<shape_flag> Flags>
constexpr auto evaluate(box_eval<Dimensions, Colour, Flags>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, glm::vec4{evaluate(Colour{}, params)},
                  shape_flag{evaluate(Flags{}, params)});
}

template <fixed W, fixed H, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using box = constant<make_box(vec2{W, H}, Colour, Flags)>;

template <fixed W, fixed H, std::size_t ParameterIndex, shape_flag Flags = shape_flag::kNone>
using box_colour_p = box_eval<constant_vec2<W, H>, parameter<ParameterIndex>, constant<Flags>>;

}  // namespace ii::geom

#endif