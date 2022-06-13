#ifndef II_GAME_COMMON_FUNCTIONAL_H
#define II_GAME_COMMON_FUNCTIONAL_H
#include <concepts>
#include <tuple>
#include <type_traits>

namespace ii {

namespace detail {
template <typename...>
struct type_list {};

struct function_traits_not_impl {
  using function_type = void;
  inline static constexpr bool is_function = false;
  inline static constexpr bool is_function_ptr = false;
  inline static constexpr bool is_function_ref = false;
  template <typename R>
  inline static constexpr bool has_return_type = false;
  template <typename... Args>
  inline static constexpr bool has_parameter_types = false;
};

template <bool Type, bool Ptr, bool Ref, typename R, typename... Args>
struct function_traits_impl {
  using function_type = R(Args...);
  using return_type = R;
  using parameter_types = type_list<Args...>;
  inline static constexpr bool is_function_type = Type;
  inline static constexpr bool is_function_ptr = Ptr;
  inline static constexpr bool is_function_ref = Ref;
};
}  // namespace detail

template <typename>
struct function_traits : detail::function_traits_not_impl {};
template <typename R, typename... Args>
struct function_traits<R(Args...)>
: detail::function_traits_impl<true, false, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)>
: detail::function_traits_impl<false, true, false, R, Args...> {};
template <typename R, typename... Args>
struct function_traits<R (&)(Args...)>
: detail::function_traits_impl<false, false, true, R, Args...> {};

template <typename T>
using function_type_t = typename function_traits<T>::function_type;
template <typename T>
using return_type_t = typename function_traits<T>::return_type;
template <typename T>
using parameter_types_t = typename function_traits<T>::parameter_types;

template <typename T>
concept function = function_traits<T>::is_function_ptr || function_traits<T>::is_function_ref;
template <typename T>
concept function_type = function_traits<T>::is_function_type;
template <typename T>
concept functional = function<T> || function_type<T>;
template <typename T, typename R>
concept function_return = functional<T> && std::is_same_v<R, return_type_t<T>>;
template <typename T, typename... Args>
concept function_parameters =
    functional<T> && std::is_same_v<detail::type_list<Args...>, parameter_types_t<T>>;
template <typename T, typename R, typename... Args>
concept function_signature = function_return<T, R> && function_parameters<T, Args...>;

template <function_type T>
using raw_function = T;
template <function_type T>
using raw_function_ptr = const T*;

namespace detail {
template <function_type T, function auto... F>
inline constexpr auto sequence_impl = std::nullopt;
template <typename R, typename... Args, function auto... F>
inline constexpr auto
    sequence_impl<R(Args...), F...> = +[](Args... args) { return (F(args...), ...); };
}  // namespace detail

template <function auto F, function auto... Rest>
requires(std::is_same_v<parameter_types_t<decltype(F)>,
                        parameter_types_t<decltype(Rest)>>&&...) inline constexpr auto sequence =
    detail::sequence_impl<function_type_t<decltype(F)>, F, Rest...>;

}  // namespace ii

#endif