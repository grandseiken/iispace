#ifndef II_GAME_COMMON_FUNCTIONAL_H
#define II_GAME_COMMON_FUNCTIONAL_H
#include "game/common/type_list.h"
#include <functional>
#include <type_traits>

namespace ii {
namespace detail {
template <typename, typename>
struct make_function_type_impl;
template <typename R, typename... Args>
struct make_function_type_impl<R, type_list<Args...>> : std::type_identity<R(Args...)> {};
}  // namespace detail

template <typename R, typename Args>
using make_function_type = typename detail::make_function_type_impl<R, Args>::type;

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
  using parameter_types = type_list<Args...>;
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
using function_type_t = typename function_traits<T>::function_type;
template <functional T>
using return_type_t = typename function_traits<T>::return_type;
template <functional T>
using parameter_types_t = typename function_traits<T>::parameter_types;

template <typename T, typename R>
concept function_return = functional<T> && std::is_same_v<R, return_type_t<T>>;
template <typename T, typename... Args>
concept function_parameters =
    functional<T> && std::is_same_v<type_list<Args...>, parameter_types_t<T>>;
template <typename T, typename R, typename... Args>
concept function_signature = function_return<T, R> && function_parameters<T, Args...>;

template <function_type T>
using function_ptr = T*;

namespace detail {
template <function_type T, member_function auto F>
inline constexpr auto unwrap_impl = nullptr;
template <typename R, typename... Args, member_function auto F>
inline constexpr auto
    unwrap_impl<R(Args...), F> = +[](Args... args) { return std::invoke(F, args...); };
template <bool MemPtr, functional auto F>
struct unwrap_if {
  static inline constexpr auto value = F;
};
template <functional auto F>
struct unwrap_if<true, F> {
  static inline constexpr auto value = unwrap_impl<function_type_t<decltype(F)>, F>;
};
}  // namespace detail

template <functional auto F>
inline constexpr auto unwrap = detail::unwrap_if<member_function<decltype(F)>, F>::value;

namespace detail {
template <function_type T, function auto... F>
inline constexpr auto sequence_impl = nullptr;
template <typename R, typename... Args, function auto... F>
inline constexpr auto
    sequence_impl<R(Args...), F...> = +[](Args... args) { return (F(args...), ...); };
}  // namespace detail

template <function auto F, function auto... Rest>
requires(std::is_same_v<parameter_types_t<decltype(F)>,
                        parameter_types_t<decltype(Rest)>>&&...) inline constexpr auto sequence =
    detail::sequence_impl<function_type_t<decltype(F)>, unwrap<F>, unwrap<Rest>...>;

}  // namespace ii

#endif