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
  std::size_t* shape_index_out = nullptr;

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
  template <typename T>
  constexpr explicit operator T() const {
    return T{0};
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

struct ball_collider_data {
  std::uint32_t radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    auto radius_fixed = ::fixed{radius};
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius_fixed * radius_fixed;
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform&, const FlagFunction auto& f) {
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

struct ngon_data {
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
  constexpr void iterate(I, const transform&, const FlagFunction auto& f) {
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

struct box_data {
  glm::uvec2 dimensions{0};
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && abs(v.x) < ::fixed{dimensions.x} && abs(v.y) < ::fixed{dimensions.y};
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform&, const FlagFunction auto& f) {
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

struct line_data {
  glm::ivec2 a{0};
  glm::ivec2 b{0};
  glm::vec4 colour{0.f};

  constexpr bool check_point(const vec2&, shape_flag) const {
    return false;
  }

  template <IterTag I>
  requires std::same_as<I, iterate_flags_t>
  constexpr void iterate(I, const transform&, const FlagFunction auto&) {}

  template <IterTag I>
  requires std::same_as<I, iterate_lines_t>
  constexpr void iterate(I, const transform& t, const LineFunction auto& f) {
    std::invoke(f, t.translate(vec2{a}).v, t.translate(vec2{b}).v, colour);
  }

  template <IterTag I>
  requires std::same_as<I, iterate_centres_t>
  constexpr void iterate(I, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v + (vec2{a} + vec2{b}) / 2, colour);
  }
};

constexpr ball_collider_data
make_ball_collider(std::uint32_t radius, shape_flag flags = shape_flag::kNone) {
  return {radius, flags};
}

constexpr ngon_data make_ngon(std::uint32_t radius, std::uint32_t sides, const glm::vec4& colour,
                              ngon_style style = ngon_style::kPolygon,
                              shape_flag flags = shape_flag::kNone) {
  return {radius, sides, colour, style, flags};
}

constexpr box_data make_box(const glm::uvec2& dimensions, const glm::vec4& colour,
                            shape_flag flags = shape_flag::kNone) {
  return {dimensions, colour, flags};
}
constexpr line_data make_line(const glm::ivec2& a, const glm::uvec2& b, const glm::vec4& colour) {
  return {a, b, colour};
}

//////////////////////////////////////////////////////////////////////////////////
// Value-expresssions.
//////////////////////////////////////////////////////////////////////////////////
template <auto V>
struct constant {};
// Special vector constants only needed for MSVC.
template <fixed X, fixed Y>
struct constant_vec2 {};
template <std::uint32_t X, std::uint32_t Y>
struct constant_uvec2 {};
template <std::int32_t X, std::int32_t Y>
struct constant_ivec2 {};
template <std::size_t N>
struct parameter {};

template <Expression<bool> Condition, typename ETrue, typename EFalse>
struct ternary {};
template <typename E>
struct negate {};
template <typename E0, typename E1>
struct multiply {};
template <typename E0, typename E1>
struct equal {};

template <auto V>
constexpr auto evaluate(constant<V>, const auto&) {
  return V;
}

template <fixed X, fixed Y>
constexpr auto evaluate(constant_vec2<X, Y>, const auto&) {
  return vec2{X, Y};
}

template <std::uint32_t X, std::uint32_t Y>
constexpr auto evaluate(constant_uvec2<X, Y>, const auto&) {
  return glm::uvec2{X, Y};
}

template <std::int32_t X, std::int32_t Y>
constexpr auto evaluate(constant_ivec2<X, Y>, const auto&) {
  return glm::ivec2{X, Y};
}

template <std::size_t N>
constexpr auto evaluate(parameter<N>, const auto& params) {
  return get<N>(params);
}

template <Expression<bool> Condition, typename ETrue, typename EFalse>
constexpr auto evaluate(ternary<Condition, ETrue, EFalse>, const auto& params) {
  return evaluate(Condition{}, params) ? evaluate(ETrue{}, params) : evaluate(EFalse{}, params);
}

template <typename E>
constexpr auto evaluate(negate<E>, const auto& params) {
  return -evaluate(E{}, params);
}

template <typename E0, typename E1>
constexpr auto evaluate(multiply<E0, E1>, const auto& params) {
  return evaluate(E0{}, params) * evaluate(E1{}, params);
}

template <typename E0, typename E1>
constexpr auto evaluate(equal<E0, E1>, const auto& params) {
  return evaluate(E0{}, params) == evaluate(E1{}, params);
}

//////////////////////////////////////////////////////////////////////////////////
// Shape-constructor-expresssions.
//////////////////////////////////////////////////////////////////////////////////
template <Expression<std::uint32_t> Radius, Expression<shape_flag> Flags>
struct ball_collider_eval {};
template <Expression<std::uint32_t> Radius, Expression<std::uint32_t> Sides,
          Expression<glm::vec4> Colour,
          Expression<ngon_style> Style = constant<ngon_style::kPolygon>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_eval {};
template <Expression<glm::uvec2> Dimensions, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_eval {};
template <Expression<glm::ivec2> A, Expression<glm::ivec2> B, Expression<glm::vec4> Colour>
struct line_eval {};

template <Expression<std::uint32_t> Radius, Expression<shape_flag> Flags>
constexpr auto evaluate(ball_collider_eval<Radius, Flags>, const auto& params) {
  return make_ball_collider(std::uint32_t{evaluate(Radius{}, params)},
                            shape_flag{evaluate(Flags{}, params)});
}

template <Expression<std::uint32_t> Radius, Expression<std::uint32_t> Sides,
          Expression<glm::vec4> Colour, Expression<ngon_style> Style, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_eval<Radius, Sides, Colour, Style, Flags>, const auto& params) {
  return make_ngon(std::uint32_t{evaluate(Radius{}, params)},
                   std::uint32_t{evaluate(Sides{}, params)}, glm::vec4{evaluate(Colour{}, params)},
                   ngon_style{evaluate(Style{}, params)}, shape_flag{evaluate(Flags{}, params)});
}

template <Expression<glm::uvec2> Dimensions, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags>
constexpr auto evaluate(box_eval<Dimensions, Colour, Flags>, const auto& params) {
  return make_box(glm::uvec2{evaluate(Dimensions{}, params)}, glm::vec4{evaluate(Colour{}, params)},
                  shape_flag{evaluate(Flags{}, params)});
}

template <Expression<glm::ivec2> A, Expression<glm::ivec2> B, Expression<glm::vec4> Colour>
constexpr auto evaluate(line_eval<A, B, Colour>, const auto& params) {
  return make_line(glm::ivec2{evaluate(A{}, params)}, glm::ivec2{evaluate(B{}, params)},
                   glm::vec4{evaluate(Colour{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Shape-expressions.
//////////////////////////////////////////////////////////////////////////////////
template <ShapeNode... Nodes>
struct compound {};

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
template <typename A, typename B>
struct pair {
  using a = A;
  using b = B;
};
template <typename Pair>
using pair_a = typename Pair::a;
template <typename Pair>
using pair_b = typename Pair::b;
template <auto Value, ShapeNode Node>
using switch_entry = pair<constant<Value>, Node>;

struct null_shape {};
template <Expression<vec2> V, ShapeNode Node>
struct translate_eval {};
template <Expression<fixed> Angle, ShapeNode Node>
struct rotate_eval {};
template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode>
struct conditional_eval {};
template <Expression<bool> Condition, ShapeNode... Nodes>
using if_eval = conditional_eval<Condition, pack<Nodes...>, null_shape>;
template <typename Value, typename... SwitchEntries>
using switch_eval =
    compound<if_eval<equal<Value, pair_a<SwitchEntries>>, pair_b<SwitchEntries>>...>;

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
  if (t.shape_index_out) {
    ++*t.shape_index_out;
  }
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
template <fixed X, fixed Y, ShapeNode... Nodes>
using translate = translate_eval<constant_vec2<X, Y>, pack<Nodes...>>;
template <fixed Angle, ShapeNode... Nodes>
using rotate = rotate_eval<constant<Angle>, pack<Nodes...>>;

template <std::size_t ParameterIndex, ShapeNode... Nodes>
using translate_p = translate_eval<parameter<ParameterIndex>, pack<Nodes...>>;
template <std::size_t ParameterIndex, ShapeNode... Nodes>
using rotate_p = rotate_eval<parameter<ParameterIndex>, pack<Nodes...>>;

template <std::size_t ParameterIndex, ShapeNode TrueNode, ShapeNode FalseNode>
using conditional_p = conditional_eval<parameter<ParameterIndex>, TrueNode, FalseNode>;
template <std::size_t ParameterIndex, ShapeNode... Nodes>
using if_p = if_eval<parameter<ParameterIndex>, Nodes...>;
template <std::size_t ParameterIndex, typename... SwitchEntries>
using switch_p = switch_eval<parameter<ParameterIndex>, SwitchEntries...>;

template <std::size_t ParameterIndex, typename ETrue, typename EFalse>
using ternary_p = ternary<parameter<ParameterIndex>, ETrue, EFalse>;
template <std::size_t ParameterIndex>
using negate_p = negate<parameter<ParameterIndex>>;
template <auto C, std::size_t ParameterIndex>
using multiply_p = multiply<constant<C>, parameter<ParameterIndex>>;

template <std::uint32_t Radius, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Radius, Flags)>;
template <std::uint32_t Radius, std::uint32_t Sides, glm::vec4 Colour,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon = constant<make_ngon(Radius, Sides, Colour, Style, Flags)>;
template <std::uint32_t W, std::uint32_t H, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using box = constant<make_box(glm::uvec2{W, H}, Colour, Flags)>;
template <std::int32_t AX, std::int32_t AY, std::int32_t BX, std::int32_t BY, glm::vec4 Colour>
using line = constant<make_line(glm::ivec2{AX, AY}, glm::ivec2{BX, BY}, Colour)>;

template <std::uint32_t Radius, std::uint32_t Sides, glm::vec4 Colour,
          shape_flag Flags = shape_flag::kNone>
using polygon = ngon<Radius, Sides, Colour, ngon_style::kPolygon, Flags>;
template <std::uint32_t Radius, std::uint32_t Sides, glm::vec4 Colour,
          shape_flag Flags = shape_flag::kNone>
using polystar = ngon<Radius, Sides, Colour, ngon_style::kPolystar, Flags>;
template <std::uint32_t Radius, std::uint32_t Sides, glm::vec4 Colour,
          shape_flag Flags = shape_flag::kNone>
using polygram = ngon<Radius, Sides, Colour, ngon_style::kPolygram, Flags>;

template <std::uint32_t Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon_colour_p = ngon_eval<constant<Radius>, constant<Sides>, parameter<ParameterIndex>,
                                constant<Style>, constant<Flags>>;
template <std::uint32_t W, std::uint32_t H, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using box_colour_p = box_eval<constant_uvec2<W, H>, parameter<ParameterIndex>, constant<Flags>>;
template <std::int32_t AX, std::int32_t AY, std::int32_t BX, std::int32_t BY,
          std::size_t ParameterIndex>
using line_colour_p =
    line_eval<constant_ivec2<AX, AY>, constant_ivec2<BX, BY>, parameter<ParameterIndex>>;

template <std::uint32_t Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygon_colour_p = ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygon, Flags>;
template <std::uint32_t Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polystar_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolystar, Flags>;
template <std::uint32_t Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygram_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygram, Flags>;

}  // namespace ii::geom

#endif