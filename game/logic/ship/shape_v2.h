#ifndef II_GAME_LOGIC_SHIP_SHAPE_V2_H
#define II_GAME_LOGIC_SHIP_SHAPE_V2_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ii {
enum class shape_flag : std::uint32_t {
  kNone = 0,
  kVulnerable = 1,
  kDangerous = 2,
  kShield = 4,
  kVulnShield = 8
};
template <>
struct bitmask_enum<shape_flag> : std::true_type {};
}  // namespace ii

namespace ii::shape {
struct iterate_lines_t {};
struct iterate_centres_t {};
inline constexpr iterate_lines_t iterate_lines;
inline constexpr iterate_centres_t iterate_centres;

struct transform {
  constexpr transform(const vec2& v = vec2{0}, fixed r = 0) : v{v}, r{r} {}
  vec2 v;
  fixed r;

  constexpr transform translate(const vec2& t) const {
    return {v + ::rotate(t, r), r};
  }

  constexpr transform rotate(fixed a) const {
    return {v, r + a};
  }
};

//////////////////////////////////////////////////////////////////////////////////
// Concepts.
//////////////////////////////////////////////////////////////////////////////////
namespace detail {
struct arbitrary_parameters {};
struct arbitrary_parameter {
  operator vec2() {
    return vec2{0, 0};
  }
  operator fixed() {
    return fixed{0};
  }
};

template <std::size_t N>
arbitrary_parameter get(const arbitrary_parameters&) {
  return {};
}
}  // namespace detail

template <typename T, typename... Args>
concept OneOf = (std::same_as<T, Args> || ...);
template <typename T>
concept IterTag = OneOf<T, iterate_lines_t, iterate_centres_t>;

template <typename T>
concept PointFunction = std::invocable<T, const vec2&, const glm::vec4&>;
template <typename T>
concept LineFunction = std::invocable<T, const vec2&, const vec2&, const glm::vec4&>;
template <typename T, typename I>
concept IterateFunction = IterTag<I> &&
    ((!std::same_as<I, iterate_lines_t> || LineFunction<T>)&&(!std::same_as<I, iterate_centres_t> ||
                                                              PointFunction<T>));

template <typename T>
concept Shape = requires(T x, transform t) {
  { x.check_point(vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  x.iterate(iterate_centres, transform{}, [](const vec2&, const glm::vec4&) {});
  x.iterate(iterate_lines, transform{}, [](const vec2&, const vec2&, const glm::vec4&) {});
};

template <typename Parameters, typename E, typename V>
concept ExpressionWithSubstitution = requires(Parameters params) {
  { E::evaluate(params) } -> std::convertible_to<V>;
};

template <typename Parameters, typename E>
concept ShapeExpressionWithSubstitution = requires(Parameters params) {
  { E::evaluate(params) } -> Shape;
};

template <typename Parameters, typename Node>
concept ShapeNodeWithSubstitution = requires(Parameters params) {
  { Node::check_point(params, vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  Node::iterate(iterate_centres, params, transform{}, [](const vec2&, const glm::vec4&) {});
  Node::iterate(iterate_lines, params, transform{},
                [](const vec2&, const vec2&, const glm::vec4&) {});
};

template <typename E, typename V>
concept Expression = ExpressionWithSubstitution<detail::arbitrary_parameters, E, V>;

template <typename E>
concept ShapeExpression = ShapeExpressionWithSubstitution<detail::arbitrary_parameters, E>;

template <typename Node>
concept ShapeNode = ShapeNodeWithSubstitution<detail::arbitrary_parameters, Node>;

//////////////////////////////////////////////////////////////////////////////////
// Atomic shapes.
//////////////////////////////////////////////////////////////////////////////////
enum class ngon_style {
  kPolygon,
  kPolystar,
  kPolygram,
};

struct ngon_data {
  std::uint32_t radius = 0;
  std::uint32_t sides = 0;
  glm::vec4 colour{0.f};
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    auto radius_fixed = ::fixed{radius};
    return +flags && (!mask || +(flags & mask)) &&
        v.x * v.x + v.y * v.y < radius_fixed * radius_fixed;
  }

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  void iterate(I, const transform& t, const LineFunction auto& f) {
    auto vertex = [&](std::uint32_t i) {
      return t.rotate(i * 2 * fixed_c::pi / sides).translate({::fixed{radius}, 0}).v;
    };

    if (style != ngon_style::kPolygram) {
      for (std::uint32_t i = 0; i < sides; ++i) {
        std::invoke(f, vertex(i), style == ngon_style::kPolygon ? vertex(i + 1) : t.v, colour);
      }
      return;
    }
    for (std::size_t i = 0; i < sides; ++i) {
      for (std::size_t j = i + 1; j < sides; ++j) {
        std::invoke(f, vertex(i), vertex(j), colour);
      }
    }
  }

  template <IterTag I>
  requires std::same_as<I, iterate_centres_t>
  void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

struct box_data {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +flags && (!mask || +(flags & mask)) && abs(v.x) < ::fixed{width} &&
        abs(v.y) < ::fixed{height};
  }

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  void iterate(I, const transform& t, const LineFunction auto& f) {
    auto a = t.translate({width, height}).v;
    auto b = t.translate({-width, height}).v;
    auto c = t.translate({-width, -height}).v;
    auto d = t.translate({width, -height}).v;

    std::invoke(f, a, b, colour);
    std::invoke(f, b, c, colour);
    std::invoke(f, c, d, colour);
    std::invoke(f, d, a, colour);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_centres_t>
  void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

//////////////////////////////////////////////////////////////////////////////////
// Value-expresssions.
//////////////////////////////////////////////////////////////////////////////////
template <auto V>
struct constant {
  static constexpr auto evaluate(const auto&) {
    return V;
  }
};

template <fixed X, fixed Y>
struct constant_vec2 {
  static constexpr auto evaluate(const auto&) {
    return vec2{X, Y};
  }
};

template <std::size_t N>
struct parameter {
  static constexpr auto evaluate(const auto& params) {
    return get<N>(params);
  }
};

//////////////////////////////////////////////////////////////////////////////////
// Shape-expressions.
//////////////////////////////////////////////////////////////////////////////////
template <ShapeExpression S>
struct shape_eval {
  template <typename Parameters>
  requires ShapeExpressionWithSubstitution<Parameters, S>
  static constexpr bool check_point(const Parameters& params, const vec2& v, shape_flag mask) {
    return S::evaluate(params).check_point(v, mask);
  }

  template <IterTag I, typename Parameters>
  requires ShapeExpressionWithSubstitution<Parameters, S>
  static void
  iterate(I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
    S::evaluate(params).iterate(tag, t, f);
  }
};

template <Expression<vec2> V, ShapeNode Node>
struct translate_eval {
  template <typename Parameters>
  requires ExpressionWithSubstitution<Parameters, V, vec2> &&
      ShapeNodeWithSubstitution<Parameters, Node>
  static constexpr bool check_point(const Parameters& params, const vec2& v, shape_flag mask) {
    return Node::check_point(params, v - vec2{V::evaluate(params)}, mask);
  }

  template <IterTag I, typename Parameters>
  requires ExpressionWithSubstitution<Parameters, V, vec2> &&
      ShapeNodeWithSubstitution<Parameters, Node>
  static void
  iterate(I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
    Node::iterate(tag, params, t.translate(vec2{V::evaluate(params)}), f);
  }
};

template <Expression<fixed> Angle, ShapeNode Node>
struct rotate_eval {
  template <typename Parameters>
  requires ExpressionWithSubstitution<Parameters, Angle, fixed> &&
      ShapeNodeWithSubstitution<Parameters, Node>
  static constexpr bool check_point(const Parameters& params, const vec2& v, shape_flag mask) {
    return Node::check_point(params, ::rotate(v, -fixed{Angle::evaluate(params)}), mask);
  }

  template <IterTag I, typename Parameters>
  requires ExpressionWithSubstitution<Parameters, Angle, fixed> &&
      ShapeNodeWithSubstitution<Parameters, Node>
  static void
  iterate(I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
    Node::iterate(tag, params, t.rotate(fixed{Angle::evaluate(params)}), f);
  }
};

template <ShapeNode... Nodes>
struct compound {
  template <typename Parameters>
  requires(ShapeNodeWithSubstitution<Parameters, Nodes>&&...) static constexpr bool check_point(
      const Parameters& params, const vec2& v, shape_flag mask) {
    return (Nodes::check_point(params, v, mask) || ...);
  }

  template <IterTag I, typename Parameters>
  requires(ShapeNodeWithSubstitution<Parameters, Nodes>&&...) static void iterate(
      I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
    (Nodes::iterate(tag, params, t, f), ...);
  }
};

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <auto V>
using shape = shape_eval<constant<V>>;

namespace detail {
template <ShapeNode Node, ShapeNode... Rest>
struct pack {
  using type = compound<Node, Rest...>;
};
template <ShapeNode Node>
struct pack<Node> {
  using type = Node;
};
}  // namespace detail

template <ShapeNode... Nodes>
using pack = typename detail::pack<Nodes...>::type;

template <fixed X, fixed Y, ShapeNode... Nodes>
using translate = translate_eval<constant_vec2<X, Y>, pack<Nodes...>>;
template <fixed Angle, ShapeNode... Nodes>
using rotate = rotate_eval<constant<Angle>, pack<Nodes...>>;

template <std::size_t N, ShapeNode... Nodes>
using translate_p = translate_eval<parameter<N>, pack<Nodes...>>;
template <std::size_t N, ShapeNode... Nodes>
using rotate_p = rotate_eval<parameter<N>, pack<Nodes...>>;

template <ngon_data S>
using ngon = shape<S>;
template <box_data S>
using box = shape<S>;

}  // namespace ii::shape

#endif