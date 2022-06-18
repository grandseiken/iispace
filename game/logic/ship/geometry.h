#ifndef II_GAME_LOGIC_SHIP_GEOMETRY_H
#define II_GAME_LOGIC_SHIP_GEOMETRY_H
#include "game/common/math.h"
#include "game/logic/ship/enums.h"
#include <glm/glm.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ii::geom {
struct iterate_flags_t {};
struct iterate_lines_t {};
struct iterate_centres_t {};
inline constexpr iterate_flags_t iterate_flags;
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
struct arbitrary_parameters {};
struct arbitrary_parameter {
  constexpr operator vec2() const {
    return vec2{0, 0};
  }
  constexpr operator fixed() const {
    return fixed{0};
  }
  constexpr explicit operator bool() const {
    return false;
  }
  constexpr arbitrary_parameter operator-() const {
    return {};
  }
};
constexpr arbitrary_parameter operator*(auto, arbitrary_parameter) {
  return {};
}
constexpr arbitrary_parameter operator*(arbitrary_parameter, auto) {
  return {};
}

template <std::size_t N>
constexpr arbitrary_parameter get(const arbitrary_parameters&) {
  return {};
}

template <typename T, typename... Args>
concept OneOf = (std::same_as<T, Args> || ...);
template <typename T>
concept IterTag = OneOf<T, iterate_flags_t, iterate_lines_t, iterate_centres_t>;

template <typename T>
concept FlagFunction = std::invocable<T, shape_flag>;
template <typename T>
concept PointFunction = std::invocable<T, const vec2&, const glm::vec4&>;
template <typename T>
concept LineFunction = std::invocable<T, const vec2&, const vec2&, const glm::vec4&>;

template <typename T, typename I>
concept IterateFunction = IterTag<I> &&((!std::same_as<I, iterate_flags_t> || FlagFunction<T>)&&(
    !std::same_as<I, iterate_lines_t> || LineFunction<T>)&&(!std::same_as<I, iterate_centres_t> ||
                                                            PointFunction<T>));

template <typename T>
concept Shape = requires(T x, transform t) {
  { x.check_point(vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  x.iterate(iterate_flags, transform{}, [](shape_flag) {});
  x.iterate(iterate_centres, transform{}, [](const vec2&, const glm::vec4&) {});
  x.iterate(iterate_lines, transform{}, [](const vec2&, const vec2&, const glm::vec4&) {});
};

template <typename Parameters, typename E, typename V>
concept ExpressionWithSubstitution = requires(Parameters params) {
  V{evaluate(E{}, params)};
};

template <typename Parameters, typename E>
concept ShapeExpressionWithSubstitution = requires(Parameters params) {
  { evaluate(E{}, params) } -> Shape;
};

template <typename Parameters, typename Node>
concept ShapeNodeWithSubstitution = requires(Parameters params) {
  { check_point(Node{}, params, vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  iterate(Node{}, iterate_flags, params, transform{}, [](shape_flag) {});
  iterate(Node{}, iterate_centres, params, transform{}, [](const vec2&, const glm::vec4&) {});
  iterate(Node{}, iterate_lines, params, transform{},
          [](const vec2&, const vec2&, const glm::vec4&) {});
};

template <typename E, typename V>
concept Expression = ExpressionWithSubstitution<arbitrary_parameters, E, V>;

template <typename E>
concept ShapeExpression = ShapeExpressionWithSubstitution<arbitrary_parameters, E>;

template <typename Node>
concept ShapeNode = ShapeNodeWithSubstitution<arbitrary_parameters, Node>;

//////////////////////////////////////////////////////////////////////////////////
// Atomic shapes.
//////////////////////////////////////////////////////////////////////////////////
enum class ngon_style {
  kPolygon,
  kPolystar,
  kPolygram,
};

struct ball_collider {
  std::uint32_t radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    auto radius_fixed = ::fixed{radius};
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius_fixed * radius_fixed;
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform& t, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  constexpr void iterate(I, const transform&, const LineFunction auto&) {}

  template <IterTag I>
  requires std::same_as<I, iterate_centres_t>
  constexpr void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, glm::vec4{0.f});
  }
};

struct ngon {
  std::uint32_t radius = 0;
  std::uint32_t sides = 0;
  glm::vec4 colour{0.f};
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    auto radius_fixed = ::fixed{radius};
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius_fixed * radius_fixed;
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform& t, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  constexpr void iterate(I, const transform& t, const LineFunction auto& f) {
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
  constexpr void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

struct box {
  glm::uvec2 dimensions{0};
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && abs(v.x) < ::fixed{dimensions.x} && abs(v.y) < ::fixed{dimensions.y};
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform& t, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  constexpr void iterate(I, const transform& t, const LineFunction auto& f) {
    auto a = t.translate({dimensions.x, dimensions.y}).v;
    auto b = t.translate({-dimensions.x, dimensions.y}).v;
    auto c = t.translate({-dimensions.x, -dimensions.y}).v;
    auto d = t.translate({dimensions.x, -dimensions.y}).v;

    std::invoke(f, a, b, colour);
    std::invoke(f, b, c, colour);
    std::invoke(f, c, d, colour);
    std::invoke(f, d, a, colour);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_centres_t>
  constexpr void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

constexpr ball_collider
make_ball_collider(std::uint32_t radius, shape_flag flags = shape_flag::kNone) {
  return {radius, flags};
}

constexpr ngon make_ngon(std::uint32_t radius, std::uint32_t sides, const glm::vec4& colour,
                         ngon_style style = ngon_style::kPolygon,
                         shape_flag flags = shape_flag::kNone) {
  return {radius, sides, colour, style, flags};
}

constexpr box make_box(const glm::uvec2& dimensions, const glm::vec4& colour,
                       shape_flag flags = shape_flag::kNone) {
  return {dimensions, colour, flags};
}

//////////////////////////////////////////////////////////////////////////////////
// Value-expresssions.
//////////////////////////////////////////////////////////////////////////////////
template <auto V>
struct constant {};
template <fixed X, fixed Y>
struct constant_vec2 {};
template <std::size_t N>
struct parameter {};

template <typename E>
struct negate {};
template <typename E0, typename E1>
struct multiply {};

template <auto V>
static constexpr auto evaluate(constant<V>, const auto&) {
  return V;
}

template <fixed X, fixed Y>
static constexpr auto evaluate(constant_vec2<X, Y>, const auto&) {
  return vec2{X, Y};
}

template <std::size_t N>
static constexpr auto evaluate(parameter<N>, const auto& params) {
  return get<N>(params);
}

template <typename E>
static constexpr auto evaluate(negate<E>, const auto& params) {
  return -evaluate(E{}, params);
}

template <typename E0, typename E1>
static constexpr auto evaluate(multiply<E0, E1>, const auto& params) {
  return evaluate(E0{}, params) * evaluate(E1{}, params);
}

//////////////////////////////////////////////////////////////////////////////////
// Shape-expressions.
//////////////////////////////////////////////////////////////////////////////////
struct null_shape {};
template <Expression<vec2> V, ShapeNode Node>
struct translate_eval {};
template <Expression<fixed> Angle, ShapeNode Node>
struct rotate_eval {};
template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode>
struct conditional_eval {};
template <ShapeNode... Nodes>
struct compound {};

template <ShapeExpression S, typename Parameters>
requires ShapeExpressionWithSubstitution<Parameters, S>
constexpr bool check_point(S, const Parameters& params, const vec2& v, shape_flag mask) {
  return evaluate(S{}, params).check_point(v, mask);
}

template <ShapeExpression S, IterTag I, typename Parameters>
requires ShapeExpressionWithSubstitution<Parameters, S>
constexpr void
iterate(S, I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
  evaluate(S{}, params).iterate(tag, t, f);
}

constexpr bool check_point(null_shape, const auto&, const vec2&, shape_flag) {
  return false;
}
template <IterTag I>
constexpr void
iterate(null_shape, I tag, const auto&, const transform&, const IterateFunction<I> auto&) {}

template <Expression<vec2> V, ShapeNode Node, typename Parameters>
requires ExpressionWithSubstitution<Parameters, V, vec2> &&
    ShapeNodeWithSubstitution<Parameters, Node>
constexpr bool
check_point(translate_eval<V, Node>, const Parameters& params, const vec2& v, shape_flag mask) {
  return check_point(Node{}, params, v - vec2{evaluate(V{}, params)}, mask);
}

template <Expression<vec2> V, ShapeNode Node, IterTag I, typename Parameters>
requires ExpressionWithSubstitution<Parameters, V, vec2> &&
    ShapeNodeWithSubstitution<Parameters, Node>
constexpr void iterate(translate_eval<V, Node>, I tag, const Parameters& params, const transform& t,
                       const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.translate(vec2{evaluate(V{}, params)}), f);
}

template <Expression<fixed> Angle, ShapeNode Node, typename Parameters>
requires ExpressionWithSubstitution<Parameters, Angle, fixed> &&
    ShapeNodeWithSubstitution<Parameters, Node>
constexpr bool
check_point(rotate_eval<Angle, Node>, const Parameters& params, const vec2& v, shape_flag mask) {
  return check_point(Node{}, params, ::rotate(v, -fixed{evaluate(Angle{}, params)}), mask);
}

template <Expression<fixed> Angle, ShapeNode Node, IterTag I, typename Parameters>
requires ExpressionWithSubstitution<Parameters, Angle, fixed> &&
    ShapeNodeWithSubstitution<Parameters, Node>
constexpr void iterate(rotate_eval<Angle, Node>, I tag, const Parameters& params,
                       const transform& t, const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.rotate(fixed{evaluate(Angle{}, params)}), f);
}

template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode, typename Parameters>
requires(
    ExpressionWithSubstitution<Parameters, Condition, bool>&&
        ShapeNodeWithSubstitution<Parameters, TrueNode>&& ShapeNodeWithSubstitution<
            Parameters,
            FalseNode>) constexpr bool check_point(conditional_eval<Condition, TrueNode, FalseNode>,
                                                   const Parameters& params, const vec2& v,
                                                   shape_flag mask) {
  return bool{evaluate(Condition{}, params)} ? check_point(TrueNode{}, params, v, mask)
                                             : check_point(FalseNode{}, params, v, mask);
}

template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode, IterTag I,
          typename Parameters>
requires(
    ExpressionWithSubstitution<Parameters, Condition, bool>&&
        ShapeNodeWithSubstitution<Parameters, TrueNode>&& ShapeNodeWithSubstitution<
            Parameters,
            FalseNode>) constexpr void iterate(conditional_eval<Condition, TrueNode, FalseNode>,
                                               I tag, const Parameters& params, const transform& t,
                                               const IterateFunction<I> auto& f) {
  if constexpr (std::is_same_v<std::remove_cvref_t<Parameters>, arbitrary_parameters>) {
    iterate(TrueNode{}, tag, params, t, f);
    iterate(FalseNode{}, tag, params, t, f);
  } else {
    if (bool{evaluate(Condition{}, params)}) {
      iterate(TrueNode{}, tag, params, t, f);
    } else {
      iterate(FalseNode{}, tag, params, t, f);
    }
  }
}

template <ShapeNode... Nodes, typename Parameters>
requires(ShapeNodeWithSubstitution<Parameters, Nodes>&&...) constexpr bool check_point(
    compound<Nodes...>, const Parameters& params, const vec2& v, shape_flag mask) {
  return (check_point(Nodes{}, params, v, mask) || ...);
}

template <ShapeNode... Nodes, IterTag I, typename Parameters>
requires(ShapeNodeWithSubstitution<Parameters, Nodes>&&...) constexpr void iterate(
    compound<Nodes...>, I tag, const Parameters& params, const transform& t,
    const IterateFunction<I> auto& f) {
  (iterate(Nodes{}, tag, params, t, f), ...);
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
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

template <std::size_t N, ShapeNode TrueNode, ShapeNode FalseNode>
using conditional_p = conditional_eval<parameter<N>, TrueNode, FalseNode>;
template <std::size_t N, ShapeNode... Nodes>
using if_p = conditional_p<N, pack<Nodes...>, null_shape>;

template <std::size_t N>
using negate_p = negate<parameter<N>>;
template <auto C, std::size_t N>
using multiply_p = multiply<constant<C>, parameter<N>>;

template <std::uint32_t Radius, shape_flag Flags>
using ball_collider_shape = constant<make_ball_collider(Radius, Flags)>;
template <std::uint32_t Radius, std::uint32_t Sides, glm::vec4 Colour,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon_shape = constant<make_ngon(Radius, Sides, Colour, Style, Flags)>;
template <std::uint32_t W, std::uint32_t H, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using box_shape = constant<make_box(glm::uvec2{W, H}, Colour, Flags)>;
template <std::uint32_t Radius, fixed Angle, glm::vec4 Colour>
using line_shape = rotate<Angle, ngon_shape<Radius, 2, Colour>>;

}  // namespace ii::geom

#endif