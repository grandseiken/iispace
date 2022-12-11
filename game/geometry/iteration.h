#ifndef II_GAME_LOGIC_GEOMETRY_ITERATION_H
#define II_GAME_LOGIC_GEOMETRY_ITERATION_H
#include "game/common/math.h"
#include "game/geometry/enums.h"
#include <cstddef>
#include <functional>
#include <type_traits>

namespace ii::render {
struct shape;
}  // namespace ii::render
namespace ii::geom {

struct iterate_flags_t {};
struct iterate_lines_t {};
struct iterate_shapes_t {};
struct iterate_centres_t {};
struct iterate_attachment_points_t {};
struct iterate_collision_t {
  shape_flag mask = shape_flag::kNone;
};

inline constexpr iterate_flags_t iterate_flags;
inline constexpr iterate_lines_t iterate_lines;
inline constexpr iterate_shapes_t iterate_shapes;
inline constexpr iterate_centres_t iterate_centres;
inline constexpr iterate_attachment_points_t iterate_attachment_points;
inline constexpr iterate_collision_t iterate_collision(shape_flag mask) {
  return iterate_collision_t{mask};
}

template <typename T, typename... Args>
concept OneOf = (std::same_as<T, Args> || ...);
template <typename T>
concept IterTag = OneOf<T, iterate_flags_t, iterate_lines_t, iterate_shapes_t, iterate_centres_t,
                        iterate_collision_t, iterate_attachment_points_t>;

template <typename T>
concept FlagFunction = std::invocable<T, shape_flag>;
template <typename T>
concept PointFunction = std::invocable<T, const vec2&, const glm::vec4&>;
template <typename T>
concept LineFunction = std::invocable<T, const vec2&, const vec2&, const glm::vec4&, float, float>;
template <typename T>
concept ShapeFunction = std::invocable<T, const render::shape&>;
template <typename T>
concept AttachmentPointFunction = std::invocable<T, std::size_t, const vec2&, const vec2&>;

template <typename T, typename U, bool B>
concept Implies = (!std::same_as<T, U> || B);
template <typename T, typename I>
concept IterateFunction = IterTag<I> && Implies<I, iterate_flags_t, FlagFunction<T>> &&
    Implies<I, iterate_lines_t, LineFunction<T>> &&
    Implies<I, iterate_shapes_t, ShapeFunction<T>> &&
    Implies<I, iterate_centres_t, PointFunction<T>> &&
    Implies<I, iterate_collision_t, FlagFunction<T>> &&
    Implies<I, iterate_attachment_points_t, AttachmentPointFunction<T>>;

template <typename T>
concept Transform = requires(const T& x) {
                      { *x } -> std::convertible_to<vec2>;
                      { x.deref_ignore_rotation() } -> std::convertible_to<vec2>;
                      { x.rotate(0_fx) } -> std::convertible_to<T>;
                      { x.translate(vec2{0}) } -> std::convertible_to<T>;
                      x.increment_index();
                    };

struct shape_data_base {
  template <IterTag I>
  constexpr void iterate(I, const Transform auto&, const IterateFunction<I> auto&) const {}
};

struct null_transform {
  constexpr null_transform() = default;
  constexpr null_transform rotate(fixed) const { return {}; }
  constexpr null_transform translate(const vec2&) const { return {}; }

  constexpr vec2 deref_ignore_rotation() const { return vec2{0}; }
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
  constexpr const vec2& deref_ignore_rotation() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr transform translate(const vec2& t) const { return {v + ::rotate(t, r), r, index_out}; }
  constexpr transform rotate(fixed a) const { return {v, r + a, index_out}; }

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
  constexpr const vec2& deref_ignore_rotation() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr legacy_transform translate(const vec2& t) const {
    return {v + ::rotate_legacy(t, r), r};
  }
  constexpr legacy_transform rotate(fixed a) const { return {v, r + a}; }
  constexpr void increment_index() const {}
};

struct convert_local_transform {
  constexpr convert_local_transform(const vec2& v, transform t = {}) : v{v}, ct{t} {}
  vec2 v;
  transform ct;

  constexpr vec2 operator*() const { return ::rotate(v - ct.v, -ct.r); }
  constexpr vec2 deref_ignore_rotation() const { return v - ct.v; }

  constexpr convert_local_transform translate(const vec2& t) const { return {v, ct.translate(t)}; }
  constexpr convert_local_transform rotate(fixed a) const { return {v, ct.rotate(a)}; }
  constexpr void increment_index() const {}
};

struct legacy_convert_local_transform {
  constexpr legacy_convert_local_transform(const vec2& v) : v{v} {}

  vec2 v;

  constexpr const vec2& deref_ignore_rotation() const { return v; }
  constexpr const vec2& operator*() const { return v; }

  constexpr legacy_convert_local_transform translate(const vec2& t) const { return {v - t}; }
  constexpr legacy_convert_local_transform rotate(fixed a) const { return {rotate_legacy(v, -a)}; }
  constexpr void increment_index() const {}
};

}  // namespace ii::geom

#endif
