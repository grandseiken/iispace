#ifndef II_GAME_COMMON_TYPE_LIST_H
#define II_GAME_COMMON_TYPE_LIST_H
#include <cstddef>
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

}  // namespace ii

#endif