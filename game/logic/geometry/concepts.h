#ifndef II_GAME_LOGIC_GEOMETRY_CONCEPTS_H
#define II_GAME_LOGIC_GEOMETRY_CONCEPTS_H
#include "game/common/math.h"
#include "game/logic/geometry/iteration.h"
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace ii::geom {

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
concept Shape = requires(T x) {
  x.iterate(iterate_flags, null_transform{}, [](shape_flag) {});
  x.iterate(iterate_lines, null_transform{}, [](const vec2&, const vec2&, const glm::vec4&) {});
  x.iterate(iterate_shapes, null_transform{}, [](const render::shape&) {});
  x.iterate(iterate_centres, null_transform{}, [](const vec2&, const glm::vec4&) {});
  x.iterate(iterate_attachment_points, null_transform{},
            [](std::size_t, const vec2&, const vec2&) {});
  x.iterate(iterate_collision(shape_flag::kNone), null_transform{}, [](shape_flag) {});
};

template <typename E, typename V, typename Parameters>
concept ExpressionWithSubstitution = requires(Parameters params) {
  V{evaluate(E{}, params)};
};

template <typename E, typename Parameters>
concept ShapeExpressionWithSubstitution = requires(Parameters params) {
  { evaluate(E{}, params) } -> Shape;
};

template <typename Node, typename Parameters>
concept ShapeNodeWithSubstitution = requires(Parameters params) {
  iterate(Node{}, iterate_flags, params, null_transform{}, [](shape_flag) {});
  iterate(Node{}, iterate_lines, params, null_transform{},
          [](const vec2&, const vec2&, const glm::vec4&) {});
  iterate(Node{}, iterate_shapes, params, null_transform{}, [](const render::shape&) {});
  iterate(Node{}, iterate_centres, params, null_transform{}, [](const vec2&, const glm::vec4&) {});
  iterate(Node{}, iterate_attachment_points, params, null_transform{},
          [](std::size_t, const vec2&, const vec2&) {});
  iterate(Node{}, iterate_collision(shape_flag::kNone), params, null_transform{},
          [](shape_flag) {});
};

template <typename E, typename V>
concept Expression = ExpressionWithSubstitution<E, V, arbitrary_parameters>;

template <typename E>
concept ShapeExpression = ShapeExpressionWithSubstitution<E, arbitrary_parameters>;

template <typename Node>
concept ShapeNode = ShapeNodeWithSubstitution<Node, arbitrary_parameters>;

}  // namespace ii::geom

#endif