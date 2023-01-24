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
struct iterate_lines_t {};
struct iterate_shapes_t {};
struct iterate_volumes_t {};
struct iterate_attachment_points_t {};
struct iterate_resolve_t {};
struct iterate_check_collision_t {
  shape_flag mask = shape_flag::kNone;
  std::variant<check_point, check_line, check_ball, check_convex> check;
};

inline constexpr iterate_flags_t iterate_flags;
inline constexpr iterate_lines_t iterate_lines;
inline constexpr iterate_shapes_t iterate_shapes;
inline constexpr iterate_volumes_t iterate_volumes;
inline constexpr iterate_attachment_points_t iterate_attachment_points;
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
concept IterTag = OneOf<T, iterate_flags_t, iterate_lines_t, iterate_shapes_t, iterate_volumes_t,
                        iterate_resolve_t, iterate_check_collision_t, iterate_attachment_points_t>;

template <typename T>
concept FlagResult = std::is_same_v<T, shape_flag&>;
template <typename T>
concept HitResult = std::is_same_v<T, hit_result&>;
template <typename T>
concept ResolveResult = std::is_same_v<T, resolve_result&>;
template <typename T>
concept VolumeFunction = std::invocable<T, const vec2&, fixed, const cvec4&, const cvec4&>;
template <typename T>
concept LineFunction = std::invocable<T, const vec2&, const vec2&, const cvec4&, float, float>;
template <typename T>
concept ShapeFunction = std::invocable<T, const render::shape&>;
template <typename T>
concept AttachmentPointFunction = std::invocable<T, std::size_t, const vec2&, const vec2&>;

template <typename T, typename U, bool B>
concept Implies = (!std::same_as<T, U> || B);
template <typename T, typename I>
concept IterateFunction = IterTag<I> && Implies<I, iterate_flags_t, FlagResult<T>> &&
    Implies<I, iterate_lines_t, LineFunction<T>> &&
    Implies<I, iterate_shapes_t, ShapeFunction<T>> &&
    Implies<I, iterate_volumes_t, VolumeFunction<T>> &&
    Implies<I, iterate_resolve_t, ResolveResult<T>> &&
    Implies<I, iterate_check_collision_t, HitResult<T>> &&
    Implies<I, iterate_attachment_points_t, AttachmentPointFunction<T>>;

template <typename T>
concept Transform = requires(const T& x) {
                      { x.rotate(0_fx) } -> std::convertible_to<T>;
                      { x.translate(vec2{0}) } -> std::convertible_to<T>;
                      x.increment_index();
                    };

struct shape_data_base {
  template <IterTag I>
  constexpr void iterate(I, const Transform auto&, IterateFunction<I> auto&&) const {}
};

struct null_transform {
  constexpr null_transform() = default;
  constexpr null_transform rotate(fixed) const { return {}; }
  constexpr null_transform translate(const vec2&) const { return {}; }

  constexpr vec2 operator*() const { return vec2{0}; }
  constexpr void increment_index() const {}
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

  // TODO: remove and replace with shape_index feature somehow?
  constexpr void increment_index() const {
    if (index_out) {
      ++*index_out;
    }
  }
};

struct legacy_transform {
  constexpr legacy_transform(const vec2& v = vec2{0}, fixed r = 0) : v{v}, r{r} {}
  vec2 v;
  fixed r;

  constexpr const vec2& operator*() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr legacy_transform translate(const vec2& t) const {
    return {v + ::rotate_legacy(t, r), r};
  }
  constexpr legacy_transform rotate(fixed a) const { return {v, r + a}; }
  constexpr void increment_index() const {}
};

struct convert_local_transform {
  constexpr convert_local_transform(transform t = {}) : ct{t} {}
  transform ct;

  constexpr std::vector<vec2> transform(std::span<const vec2> vs) const {
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
  constexpr void increment_index() const {}
};

struct legacy_convert_local_transform {
  constexpr legacy_convert_local_transform(const vec2& v) : v{v} {}

  vec2 v;

  constexpr vec2 transform(const vec2&) const { return v; }
  constexpr vec2 transform_ignore_rotation(const vec2&) const { return v; }
  constexpr vec2 inverse_transform(const vec2&) const { return vec2{0}; }

  constexpr legacy_convert_local_transform translate(const vec2& t) const { return {v - t}; }
  constexpr legacy_convert_local_transform rotate(fixed a) const { return {rotate_legacy(v, -a)}; }
  constexpr void increment_index() const {}
};

}  // namespace ii::geom

#endif
