#ifndef II_GAME_COMMON_FUNCTIONAL_H
#define II_GAME_COMMON_FUNCTIONAL_H
#include <concepts>
#include <tuple>
#include <type_traits>

namespace ii {
template <typename...>
struct type_list {};

template <typename>
inline constexpr bool is_type_list_v = false;
template <typename... Args>
inline constexpr bool is_type_list_v<type_list<Args...>> = true;
template <typename T>
concept is_type_list = is_type_list_v<T>;

template <typename>
inline constexpr std::size_t type_list_size = 0;
template <typename... Args>
inline constexpr std::size_t type_list_size<type_list<Args...>> = sizeof...(Args);

namespace detail {
template <typename A, typename B>
struct type_list_concat_impl : std::type_identity<type_list<A, B>> {};
template <typename... Args, typename B>
struct type_list_concat_impl<type_list<Args...>, B> : std::type_identity<type_list<Args..., B>> {};
template <typename A, typename... Brgs>
struct type_list_concat_impl<A, type_list<Brgs...>> : std::type_identity<type_list<A, Brgs...>> {};
template <typename... Args, typename... Brgs>
struct type_list_concat_impl<type_list<Args...>, type_list<Brgs...>>
: std::type_identity<type_list<Args..., Brgs...>> {};
}  // namespace detail

template <typename A, typename B>
using type_list_concat = typename detail::type_list_concat_impl<A, B>::type;

namespace detail {
template <typename, template <typename> typename F>
struct type_list_filter_impl;
template <template <typename> typename F>
struct type_list_filter_impl<type_list<>, F> : std::type_identity<type_list<>> {};
template <typename T, typename... Args, template <typename> typename F>
struct type_list_filter_impl<type_list<T, Args...>, F>
: std::type_identity<
      type_list_concat<std::conditional_t<F<T>::value, T, type_list<>>,
                       typename type_list_filter_impl<type_list<Args...>, F>::type>> {};
}  // namespace detail

template <typename T, template <typename> typename F>
using type_list_filter = typename detail::type_list_filter_impl<T, F>::type;

namespace detail {
template <typename, template <typename> typename F>
struct type_list_drop_front_impl;
template <template <typename> typename F>
struct type_list_drop_front_impl<type_list<>, F> : std::type_identity<type_list<>> {};
template <typename T, typename... Args, template <typename> typename F>
struct type_list_drop_front_impl<type_list<T, Args...>, F>
: std::type_identity<std::conditional_t<
      F<T>::value, typename type_list_drop_front_impl<type_list<Args...>, F>::type,
      type_list<T, Args...>>> {};

template <typename, std::size_t>
struct type_list_first_n_impl;
template <typename... Args>
struct type_list_first_n_impl<type_list<Args...>, 0> : std::type_identity<type_list<>> {};
template <typename T, typename... Args, std::size_t N>
requires(N > 0) struct type_list_first_n_impl<type_list<T, Args...>, N>
: std::type_identity<
      type_list_concat<T, typename type_list_first_n_impl<type_list<Args...>, N - 1>::type>> {
};
}  // namespace detail

template <typename T, template <typename> typename F>
using type_list_drop_front = typename detail::type_list_drop_front_impl<T, F>::type;
template <typename T, std::size_t N>
using type_list_first_n = typename detail::type_list_first_n_impl<T, N>::type;

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
    functional<T> && std::is_same_v<type_list<Args...>, parameter_types_t<T>>;
template <typename T, typename R, typename... Args>
concept function_signature = function_return<T, R> && function_parameters<T, Args...>;

template <function_type T>
using function_ptr = T*;

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
    detail::sequence_impl<function_type_t<decltype(F)>, F, Rest...>;

}  // namespace ii

#endif