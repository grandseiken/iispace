#ifndef II_GAME_COMMON_FUNCTIONAL_H
#define II_GAME_COMMON_FUNCTIONAL_H
#include "game/common/type_list.h"
#include <functional>
#include <type_traits>

namespace ii {
namespace detail {

struct function_traits_not_impl {
  inline static constexpr bool is_function = false;
  inline static constexpr bool is_function_ptr = false;
  inline static constexpr bool is_function_ref = false;
  inline static constexpr bool is_member_function_ptr = false;
};

template <bool Type, bool Ptr, bool Ref, bool MemPtr, typename R, typename... Args>
struct function_traits_impl {
  using function_type = R(Args...);
  using return_type = R;
  using parameter_types = tl::list<Args...>;
  inline static constexpr bool is_function_type = Type;
  inline static constexpr bool is_function_ptr = Ptr;
  inline static constexpr bool is_function_ref = Ref;
  inline static constexpr bool is_member_function_ptr = MemPtr;
};
}  // namespace detail

template <typename>
struct function_traits : detail::function_traits_not_impl {};
template <typename R, typename... Args>
struct function_traits<R(Args...)>
: detail::function_traits_impl<true, false, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
: detail::function_traits_impl<false, true, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (&)(Args...)>
: detail::function_traits_impl<false, false, true, false, R, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)>
: detail::function_traits_impl<false, false, false, true, R, C&, Args...> {};
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const>
: detail::function_traits_impl<false, false, false, true, R, const C&, Args...> {};

template <typename T>
concept member_function = function_traits<T>::is_member_function_ptr;
template <typename T>
concept function = function_traits<T>::is_function_ptr || function_traits<T>::is_function_ref ||
    member_function<T>;
template <typename T>
concept function_type = function_traits<T>::is_function_type;
template <typename T>
concept functional = function<T> || function_type<T>;

template <functional T>
using function_type_of = typename function_traits<T>::function_type;
template <functional T>
using return_type_of = typename function_traits<T>::return_type;
template <functional T>
using parameter_types_of = typename function_traits<T>::parameter_types;

template <function_type T>
using function_ptr = T*;

namespace detail {
template <member_function auto F, typename... Args>
consteval auto unwrap_create(tl::list<Args...>) {
  struct s {
    inline static constexpr decltype(auto) f(Args... args) {
      return std::invoke(F, args...);
    }
  };
  return &s::f;
}
template <function auto F, bool MemPtr = member_function<decltype(F)>>
struct unwrap_if {
  static inline constexpr auto value = F;
};
template <function auto F>
struct unwrap_if<F, true> {
  static inline constexpr auto value = unwrap_create<F>(parameter_types_of<decltype(F)>{});
};
}  // namespace detail

template <function auto F>
inline constexpr auto unwrap = detail::unwrap_if<F>::value;

namespace detail {
template <function auto F, typename R, typename... UsedSourceArgs, typename... UnusedSourceArgs,
          typename... UsedTargetArgs, typename... UnusedTargetArgs>
consteval auto cast_create(tl::list<UsedSourceArgs...>, tl::list<UnusedSourceArgs...>,
                           tl::list<UsedTargetArgs...>, tl::list<UnusedTargetArgs...>) {
  struct s {
    inline static constexpr decltype(auto) f(UsedTargetArgs... args, UnusedTargetArgs...) {
      return static_cast<R>(F(static_cast<UsedSourceArgs>(args)..., UnusedSourceArgs{}...));
    }
  };
  return &s::f;
}

template <typename T>
using is_default_constructible = std::is_constructible<T>;
template <typename... SourceArgs, typename... TargetArgs>
consteval bool all_constructible(tl::list<SourceArgs...>, tl::list<TargetArgs...>) {
  return (std::is_constructible_v<SourceArgs, TargetArgs> && ...);
}

template <functional Source, functional Target>
struct cast_impl {
  using source_args = parameter_types_of<Source>;
  using target_args = parameter_types_of<Target>;
  inline static constexpr auto n =
      tl::size<source_args> < tl::size<target_args> ? tl::size<source_args> : tl::size<target_args>;
  using used_source_args = tl::sublist<source_args, 0, n>;
  using used_target_args = tl::sublist<target_args, 0, n>;
  using unused_source_args = tl::sublist<source_args, n>;
  using unused_target_args = tl::sublist<target_args, n>;

  inline static constexpr bool return_type_convertible =
      std::is_same_v<return_type_of<Target>, void> ||
      std::is_constructible_v<return_type_of<Target>, return_type_of<Source>>;
  inline static constexpr bool matching_parameters_convertible =
      all_constructible(used_source_args{}, used_target_args{});
  inline static constexpr bool missing_parameters_default_constructible =
      tl::all_of<unused_source_args, is_default_constructible>;

  template <function auto F>
  inline static constexpr auto
      cast = cast_create<F, return_type_of<Target>>(used_source_args{}, unused_source_args{},
                                                    used_target_args{}, unused_target_args{});
};

template <function_type T, function auto F,
          bool IsSame = std::is_same_v<T, function_type_of<decltype(F)>>>
struct cast_if {
  static inline constexpr auto value = F;
};
template <function_type T, function auto F>
struct cast_if<T, F, false> {
  static inline constexpr auto value = cast_impl<decltype(F), T>::template cast<F>;
};

template <function auto... F, typename... Args>
consteval auto sequence_create(tl::list<Args...>) {
  struct s {
    inline static constexpr decltype(auto) f(Args... args) {
      return (F(args...), ...);
    }
  };
  return &s::f;
}
}  // namespace detail

template <typename T, typename... Rest>
concept same_parameters = functional<T> &&(functional<Rest>&&...) &&
    (std::is_same_v<parameter_types_of<T>, parameter_types_of<Rest>> && ...);

template <typename Source, typename Target>
concept castable_to = functional<Source> && functional<Target> &&
    detail::cast_impl<Source, Target>::return_type_convertible &&
    detail::cast_impl<Source, Target>::matching_parameters_convertible &&
    detail::cast_impl<Source, Target>::missing_parameters_default_constructible;

template <function auto F, function auto... Rest>
requires same_parameters<decltype(F), decltype(Rest)...>
inline constexpr auto sequence =
    detail::sequence_create<unwrap<F>, unwrap<Rest>...>(parameter_types_of<decltype(F)>{});

template <function_type T, function auto F>
requires castable_to<decltype(F), T>
inline constexpr auto cast = detail::cast_if<T, unwrap<F>>::value;

// TODO: bind; compose; rearrange.

}  // namespace ii

#endif