#ifndef II_GAME_GEOMETRY_ITERATION_H
#define II_GAME_GEOMETRY_ITERATION_H
#include "game/common/math.h"
#include "game/geometry/enums.h"
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <variant>

namespace ii::render {
struct shape;
}  // namespace ii::render
namespace ii::geom {

struct check_point {
  vec2 v{0};
};

struct check_line {
  vec2 a{0};
  vec2 b{0};
};

struct check_ball {
  vec2 c{0};
  fixed r = 0;
};

struct check_convex {
  std::span<const vec2> vs;
};

struct hit_result {
  shape_flag mask = shape_flag::kNone;
  std::vector<vec2> shape_centres;

  void add(shape_flag hit_mask) { mask |= hit_mask; }

  void add(shape_flag hit_mask, const vec2& v) {
    mask |= hit_mask;
    shape_centres.emplace_back(v);
  }
};

struct resolve_result;

struct iterate_flags_t {};
struct iterate_resolve_t {};
struct iterate_check_collision_t {
  shape_flag mask = shape_flag::kNone;
  std::variant<check_point, check_line, check_ball, check_convex> check;
};

inline constexpr iterate_flags_t iterate_flags;
inline constexpr iterate_resolve_t iterate_resolve() {
  return {};
}
inline constexpr iterate_check_collision_t iterate_check_point(shape_flag mask, const vec2& v) {
  return {mask, check_point{v}};
}
inline constexpr iterate_check_collision_t
iterate_check_line(shape_flag mask, const vec2& a, const vec2& b) {
  return {mask, check_line{a, b}};
}
inline constexpr iterate_check_collision_t
iterate_check_ball(shape_flag mask, const vec2& c, fixed r) {
  return {mask, check_ball{c, r}};
}
inline constexpr iterate_check_collision_t
iterate_check_convex(shape_flag mask, std::span<const vec2> vs) {
  return {mask, check_convex{vs}};
}

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

struct transform {
  constexpr transform(const vec2& v = vec2{0}, fixed r = 0, std::size_t* index_out = nullptr)
  : v{v}, r{r}, index_out{index_out} {}
  vec2 v;
  fixed r;
  std::size_t* index_out = nullptr;

  constexpr const vec2& operator*() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr transform translate(const vec2& t) const { return {v + ::rotate(t, r), r, index_out}; }
  constexpr transform rotate(fixed a) const { return {v, r + a, index_out}; }
};

struct convert_local_transform {
  constexpr convert_local_transform(transform t = {}) : ct{t} {}
  transform ct;

  std::vector<vec2> transform(std::span<const vec2> vs) const {
    std::vector<vec2> r;
    r.reserve(vs.size());
    for (const auto& v : vs) {
      r.emplace_back(transform(v));
    }
    return r;
  }
  constexpr vec2 transform(const vec2& v) const { return ::rotate(v - ct.v, -ct.r); }
  constexpr vec2 transform_ignore_rotation(const vec2& v) const { return v - ct.v; }
  constexpr vec2 inverse_transform(const vec2& v) const { return ::rotate(v, ct.r) + ct.v; }

  constexpr convert_local_transform translate(const vec2& t) const { return {ct.translate(t)}; }
  constexpr convert_local_transform rotate(fixed a) const { return {ct.rotate(a)}; }
};

struct legacy_convert_local_transform {
  constexpr legacy_convert_local_transform(const vec2& v) : v{v} {}

  vec2 v;

  constexpr vec2 transform(const vec2&) const { return v; }
  constexpr vec2 transform_ignore_rotation(const vec2&) const { return v; }
  constexpr vec2 inverse_transform(const vec2&) const { return vec2{0}; }

  constexpr legacy_convert_local_transform translate(const vec2& t) const { return {v - t}; }
  constexpr legacy_convert_local_transform rotate(fixed a) const { return {rotate_legacy(v, -a)}; }
};

}  // namespace ii::geom

#endif
