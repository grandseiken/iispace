#ifndef II_GAME_GEOMETRY_ITERATION_H
#define II_GAME_GEOMETRY_ITERATION_H
#include "game/common/math.h"
#include "game/geom2/resolve.h"
#include "game/geom2/types.h"
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <variant>

namespace ii::render {
struct shape;
}  // namespace ii::render
namespace ii::geom {

using geom2::check_ball_t;
using geom2::check_convex_t;
using geom2::check_line_t;
using geom2::check_point_t;
using geom2::check_t;
using geom2::hit_result;
using geom2::resolve_result;

struct iterate_flags_t {};
struct iterate_resolve_t {};
using iterate_check_collision_t = geom2::check_t;

inline constexpr iterate_flags_t iterate_flags;
inline constexpr iterate_resolve_t iterate_resolve() {
  return {};
}
using geom2::check_ball;
using geom2::check_convex;
using geom2::check_line;
using geom2::check_point;

template <typename T, typename... Args>
concept OneOf = (std::same_as<T, Args> || ...);
template <typename T>
concept IterTag = OneOf<T, iterate_flags_t, iterate_resolve_t, iterate_check_collision_t>;

template <typename T>
concept FlagResult = std::is_same_v<T, shape_flag&>;
template <typename T>
concept HitResult = std::is_same_v<T, hit_result&>;
template <typename T>
concept ResolveResult = std::is_same_v<T, resolve_result&>;

template <typename T, typename U, bool B>
concept Implies = (!std::same_as<T, U> || B);
template <typename T, typename I>
concept IterateFunction = IterTag<I> && Implies<I, iterate_flags_t, FlagResult<T>> &&
    Implies<I, iterate_resolve_t, ResolveResult<T>> &&
    Implies<I, iterate_check_collision_t, HitResult<T>>;

template <typename T>
concept Transformer = requires(const T& x) {
                        { x.rotate(0_fx) } -> std::convertible_to<T>;
                        { x.translate(vec2{0}) } -> std::convertible_to<T>;
                      };

struct shape_data_base {
  template <IterTag I>
  constexpr void iterate(I, const Transformer auto&, IterateFunction<I> auto&&) const {}
};

struct null_transform {
  constexpr null_transform() = default;
  constexpr null_transform rotate(fixed) const { return {}; }
  constexpr null_transform translate(const vec2&) const { return {}; }

  constexpr vec2 operator*() const { return vec2{0}; }
};

using geom2::convert_local_transform;
using geom2::legacy_convert_local_transform;
using geom2::transform;

}  // namespace ii::geom

#endif
