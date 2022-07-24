#ifndef II_GAME_LOGIC_GEOMETRY_ITERATION_H
#define II_GAME_LOGIC_GEOMETRY_ITERATION_H
#include "game/common/math.h"
#include "game/logic/geometry/enums.h"
#include <cstddef>
#include <type_traits>

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
  constexpr transform(const vec2& v = vec2{0}, fixed r = 0, std::size_t* shape_index_out = nullptr)
  : v{v}, r{r}, shape_index_out{shape_index_out} {}
  vec2 v;
  fixed r;
  std::size_t* shape_index_out = nullptr;

  constexpr transform translate(const vec2& t) const {
    return {v + ::rotate(t, r), r, shape_index_out};
  }

  constexpr transform rotate(fixed a) const {
    return {v, r + a, shape_index_out};
  }
};

}  // namespace ii::geom

#endif