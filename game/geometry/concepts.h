#ifndef II_GAME_GEOMETRY_CONCEPTS_H
#define II_GAME_GEOMETRY_CONCEPTS_H
#include "game/common/math.h"
#include "game/geometry/iteration.h"
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace ii::geom {

struct shape_node {};
struct arbitrary_parameters {};
struct arbitrary_parameter {
  template <typename T>
  constexpr explicit operator T() const {
    return T{0};
  }
  constexpr arbitrary_parameter operator-() const { return {}; }
  constexpr arbitrary_parameter operator[](std::size_t) const { return {}; }
};
constexpr arbitrary_parameter operator*(auto, arbitrary_parameter) {
  return {};
}
constexpr arbitrary_parameter operator*(arbitrary_parameter, auto) {
  return {};
}
constexpr bool operator==(arbitrary_parameter, auto) {
  return true;
}
constexpr bool operator==(auto, arbitrary_parameter) {
  return true;
}

template <std::size_t N>
constexpr arbitrary_parameter get(const arbitrary_parameters&) {
  return {};
}

template <typename T>
concept Shape =
    requires(T x, hit_result& hit) {
      x.iterate(iterate_flags, null_transform{}, [](shape_flag) {});
      x.iterate(iterate_lines, null_transform{},
                [](const vec2&, const vec2&, const cvec4&, float, float) {});
      x.iterate(iterate_shapes, null_transform{}, [](const render::shape&) {});
      x.iterate(iterate_volumes, null_transform{},
                [](const vec2&, fixed, const cvec4&, const cvec4&) {});
      x.iterate(iterate_attachment_points, null_transform{},
                [](std::size_t, const vec2&, const vec2&) {});
      x.iterate(iterate_check_point(shape_flag::kNone, vec2{}), convert_local_transform{}, hit);
    };

template <typename E, typename V, typename Parameters>
concept ExpressionWithSubstitution = requires(Parameters params) { V{evaluate(E{}, params)}; };

template <typename E, typename Parameters>
concept ShapeExpressionWithSubstitution = requires(Parameters params) {
                                            { evaluate(E{}, params) } -> Shape;
                                          };

template <typename E, typename V>
concept Expression = ExpressionWithSubstitution<E, V, arbitrary_parameters>;

template <typename E>
concept ShapeExpression = ShapeExpressionWithSubstitution<E, arbitrary_parameters>;

template <typename Node>
concept ShapeNode = std::is_base_of<shape_node, Node>::value || ShapeExpression<Node>;

}  // namespace ii::geom

#endif