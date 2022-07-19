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
struct iterate_attachment_points_t {};
inline constexpr iterate_flags_t iterate_flags;
inline constexpr iterate_lines_t iterate_lines;
inline constexpr iterate_centres_t iterate_centres;
inline constexpr iterate_attachment_points_t iterate_attachment_points;

template <typename T, typename... Args>
concept OneOf = (std::same_as<T, Args> || ...);
template <typename T>
concept IterTag =
    OneOf<T, iterate_flags_t, iterate_lines_t, iterate_centres_t, iterate_attachment_points_t>;

template <typename T>
concept FlagFunction = std::invocable<T, shape_flag>;
template <typename T>
concept PointFunction = std::invocable<T, const vec2&, const glm::vec4&>;
template <typename T>
concept LineFunction = std::invocable<T, const vec2&, const vec2&, const glm::vec4&>;
template <typename T>
concept AttachmentPointFunction = std::invocable<T, std::size_t, const vec2&, const vec2&>;

template <typename T, typename U, bool B>
concept Implies = (!std::same_as<T, U> || B);
template <typename T, typename I>
concept IterateFunction = IterTag<I> && Implies<I, iterate_flags_t, FlagFunction<T>> &&
    Implies<I, iterate_lines_t, LineFunction<T>> &&
    Implies<I, iterate_centres_t, PointFunction<T>> &&
    Implies<I, iterate_attachment_points_t, AttachmentPointFunction<T>>;

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

template <typename T>
concept Shape = requires(T x) {
  { x.check_point(vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  x.iterate(iterate_flags, transform{}, [](shape_flag) {});
  x.iterate(iterate_centres, transform{}, [](const vec2&, const glm::vec4&) {});
  x.iterate(iterate_lines, transform{}, [](const vec2&, const vec2&, const glm::vec4&) {});
  x.iterate(iterate_attachment_points, transform{}, [](std::size_t, const vec2&, const vec2&) {});
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
  { check_point(Node{}, params, vec2{}, shape_flag{}) } -> std::convertible_to<bool>;
  iterate(Node{}, iterate_flags, params, transform{}, [](shape_flag) {});
  iterate(Node{}, iterate_centres, params, transform{}, [](const vec2&, const glm::vec4&) {});
  iterate(Node{}, iterate_lines, params, transform{},
          [](const vec2&, const vec2&, const glm::vec4&) {});
  iterate(Node{}, iterate_attachment_points, params, transform{},
          [](std::size_t, const vec2&, const vec2&) {});
};

template <typename E, typename V>
concept Expression = ExpressionWithSubstitution<E, V, arbitrary_parameters>;

template <typename E>
concept ShapeExpression = ShapeExpressionWithSubstitution<E, arbitrary_parameters>;

template <typename Node>
concept ShapeNode = ShapeNodeWithSubstitution<Node, arbitrary_parameters>;

//////////////////////////////////////////////////////////////////////////////////
// Atomic shapes.
//////////////////////////////////////////////////////////////////////////////////
enum class ngon_style {
  kPolygon,
  kPolystar,
  kPolygram,
};

struct shape_data_base {
  constexpr bool check_point(const vec2&, shape_flag) const {
    return false;
  }
  template <IterTag I>
  constexpr void iterate(I, const transform&, const IterateFunction<I> auto&) {}
};

struct ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius * radius;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, glm::vec4{0.f});
  }
};

struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  glm::vec4 colour{0.f};
  ngon_style style = ngon_style::kPolygon;
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius * radius;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) {
    auto vertex = [&](std::uint32_t i) {
      return t.rotate(i * 2 * fixed_c::pi / sides).translate({radius, 0}).v;
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

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

struct box_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  glm::vec4 colour{0.f};
  shape_flag flags = shape_flag::kNone;

  constexpr bool check_point(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && abs(v.x) < dimensions.x && abs(v.y) < dimensions.y;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) {
    auto a = t.translate({dimensions.x, dimensions.y}).v;
    auto b = t.translate({-dimensions.x, dimensions.y}).v;
    auto c = t.translate({-dimensions.x, -dimensions.y}).v;
    auto d = t.translate({dimensions.x, -dimensions.y}).v;

    std::invoke(f, a, b, colour);
    std::invoke(f, b, c, colour);
    std::invoke(f, c, d, colour);
    std::invoke(f, d, a, colour);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v, colour);
  }
};

struct line_data : shape_data_base {
  using shape_data_base::check_point;
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  glm::vec4 colour{0.f};

  constexpr void iterate(iterate_lines_t, const transform& t, const LineFunction auto& f) {
    std::invoke(f, t.translate(a).v, t.translate(b).v, colour);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) {
    std::invoke(f, t.v + (a + b) / 2, colour);
  }
};

struct attachment_point_data : shape_data_base {
  using shape_data_base::check_point;
  using shape_data_base::iterate;
  std::size_t index{0};
  vec2 d{0};

  constexpr void
  iterate(iterate_attachment_points_t, const transform& t, const AttachmentPointFunction auto& f) {
    std::invoke(f, index, t.v, t.translate(d).v - t.v);
  }
};

constexpr ball_collider_data
make_ball_collider(fixed radius, shape_flag flags = shape_flag::kNone) {
  return {{}, radius, flags};
}

constexpr ngon_data make_ngon(fixed radius, std::uint32_t sides, const glm::vec4& colour,
                              ngon_style style = ngon_style::kPolygon,
                              shape_flag flags = shape_flag::kNone) {
  return {{}, radius, sides, colour, style, flags};
}

constexpr box_data
make_box(const vec2& dimensions, const glm::vec4& colour, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, colour, flags};
}

constexpr line_data make_line(const vec2& a, const vec2& b, const glm::vec4& colour) {
  return {{}, a, b, colour};
}

constexpr attachment_point_data make_attachment_point(std::size_t index, const vec2& d) {
  return {{}, index, d};
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
template <Expression<fixed> Radius, Expression<shape_flag> Flags>
struct ball_collider_eval {};
template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<glm::vec4> Colour,
          Expression<ngon_style> Style = constant<ngon_style::kPolygon>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_eval {};
template <Expression<vec2> Dimensions, Expression<glm::vec4> Colour,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_eval {};
template <Expression<vec2> A, Expression<vec2> B, Expression<glm::vec4> Colour>
struct line_eval {};
template <Expression<std::size_t> Index, Expression<vec2> Direction = constant<vec2{0}>>
struct attachment_point_eval {};

template <Expression<fixed> Radius, Expression<shape_flag> Flags>
constexpr auto evaluate(ball_collider_eval<Radius, Flags>, const auto& params) {
  return make_ball_collider(fixed{evaluate(Radius{}, params)},
                            shape_flag{evaluate(Flags{}, params)});
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides, Expression<glm::vec4> Colour,
          Expression<ngon_style> Style, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_eval<Radius, Sides, Colour, Style, Flags>, const auto& params) {
  return make_ngon(fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
                   glm::vec4{evaluate(Colour{}, params)}, ngon_style{evaluate(Style{}, params)},
                   shape_flag{evaluate(Flags{}, params)});
}

template <Expression<vec2> Dimensions, Expression<glm::vec4> Colour, Expression<shape_flag> Flags>
constexpr auto evaluate(box_eval<Dimensions, Colour, Flags>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, glm::vec4{evaluate(Colour{}, params)},
                  shape_flag{evaluate(Flags{}, params)});
}

template <Expression<vec2> A, Expression<vec2> B, Expression<glm::vec4> Colour>
constexpr auto evaluate(line_eval<A, B, Colour>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   glm::vec4{evaluate(Colour{}, params)});
}

template <Expression<std::size_t> Index, Expression<vec2> Direction>
constexpr auto evaluate(attachment_point_eval<Index, Direction>, const auto& params) {
  return make_attachment_point(std::size_t{evaluate(Index{}, params)},
                               vec2{evaluate(Direction{}, params)});
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
template <IterTag DisableI, ShapeNode Node>
struct disable_iteration {};
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

template <typename Parameters, ShapeExpressionWithSubstitution<Parameters> S>
constexpr bool check_point(S, const Parameters& params, const vec2& v, shape_flag mask) {
  return evaluate(S{}, params).check_point(v, mask);
}

template <IterTag I, typename Parameters, ShapeExpressionWithSubstitution<Parameters> S>
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

template <IterTag DisableI, typename Parameters, ShapeNodeWithSubstitution<Parameters> Node>
constexpr bool check_point(disable_iteration<DisableI, Node>, const Parameters& params,
                           const vec2& v, shape_flag mask) {
  return check_point(Node{}, params, v, mask);
}

template <IterTag DisableI, IterTag I, typename Parameters,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(disable_iteration<DisableI, Node>, I tag, const Parameters& params,
                       const transform& t, const IterateFunction<I> auto& f) {
  if constexpr (!std::is_same_v<DisableI, I>) {
    iterate(Node{}, tag, params, t, f);
  }
}

template <typename Parameters, ExpressionWithSubstitution<vec2, Parameters> V,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr bool
check_point(translate_eval<V, Node>, const Parameters& params, const vec2& v, shape_flag mask) {
  return check_point(Node{}, params, v - vec2{evaluate(V{}, params)}, mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<vec2, Parameters> V,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(translate_eval<V, Node>, I tag, const Parameters& params, const transform& t,
                       const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.translate(vec2{evaluate(V{}, params)}), f);
}

template <typename Parameters, ExpressionWithSubstitution<fixed, Parameters> Angle,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr bool
check_point(rotate_eval<Angle, Node>, const Parameters& params, const vec2& v, shape_flag mask) {
  return check_point(Node{}, params, ::rotate(v, -fixed{evaluate(Angle{}, params)}), mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<fixed, Parameters> Angle,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(rotate_eval<Angle, Node>, I tag, const Parameters& params,
                       const transform& t, const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.rotate(fixed{evaluate(Angle{}, params)}), f);
}

template <typename Parameters, ExpressionWithSubstitution<bool, Parameters> Condition,
          ShapeNodeWithSubstitution<Parameters> TrueNode,
          ShapeNodeWithSubstitution<Parameters> FalseNode>
constexpr bool check_point(conditional_eval<Condition, TrueNode, FalseNode>,
                           const Parameters& params, const vec2& v, shape_flag mask) {
  return bool{evaluate(Condition{}, params)} ? check_point(TrueNode{}, params, v, mask)
                                             : check_point(FalseNode{}, params, v, mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<bool, Parameters> Condition,
          ShapeNodeWithSubstitution<Parameters> TrueNode,
          ShapeNodeWithSubstitution<Parameters> FalseNode>
constexpr void
iterate(conditional_eval<Condition, TrueNode, FalseNode>, I tag, const Parameters& params,
        const transform& t, const IterateFunction<I> auto& f) {
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

template <typename Parameters, ShapeNodeWithSubstitution<Parameters>... Nodes>
constexpr bool
check_point(compound<Nodes...>, const Parameters& params, const vec2& v, shape_flag mask) {
  return (check_point(Nodes{}, params, v, mask) || ...);
}

template <IterTag I, typename Parameters, ShapeNodeWithSubstitution<Parameters>... Nodes>
constexpr void iterate(compound<Nodes...>, I tag, const Parameters& params, const transform& t,
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

template <fixed Radius, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Radius, Flags)>;
template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon = constant<make_ngon(Radius, Sides, Colour, Style, Flags)>;
template <fixed W, fixed H, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using box = constant<make_box(vec2{W, H}, Colour, Flags)>;
template <fixed AX, fixed AY, fixed BX, fixed BY, glm::vec4 Colour>
using line = constant<make_line(vec2{AX, AY}, vec2{BX, BY}, Colour)>;
template <std::size_t Index, fixed DX, fixed DY>
using attachment_point = constant<make_attachment_point(Index, vec2{DX, DY})>;

template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polygon = ngon<Radius, Sides, Colour, ngon_style::kPolygon, Flags>;
template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polystar = ngon<Radius, Sides, Colour, ngon_style::kPolystar, Flags>;
template <fixed Radius, std::uint32_t Sides, glm::vec4 Colour, shape_flag Flags = shape_flag::kNone>
using polygram = ngon<Radius, Sides, Colour, ngon_style::kPolygram, Flags>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          ngon_style Style = ngon_style::kPolygon, shape_flag Flags = shape_flag::kNone>
using ngon_colour_p = ngon_eval<constant<Radius>, constant<Sides>, parameter<ParameterIndex>,
                                constant<Style>, constant<Flags>>;
template <fixed W, fixed H, std::size_t ParameterIndex, shape_flag Flags = shape_flag::kNone>
using box_colour_p = box_eval<constant_vec2<W, H>, parameter<ParameterIndex>, constant<Flags>>;
template <fixed AX, fixed AY, fixed BX, fixed BY, std::size_t ParameterIndex>
using line_colour_p =
    line_eval<constant_vec2<AX, AY>, constant_vec2<BX, BY>, parameter<ParameterIndex>>;

template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygon_colour_p = ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygon, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polystar_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolystar, Flags>;
template <fixed Radius, std::uint32_t Sides, std::size_t ParameterIndex,
          shape_flag Flags = shape_flag::kNone>
using polygram_colour_p =
    ngon_colour_p<Radius, Sides, ParameterIndex, ngon_style::kPolygram, Flags>;

}  // namespace ii::geom

#endif