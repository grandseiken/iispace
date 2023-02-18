#ifndef II_GAME_COMMON_VARIANT_SWITCH_H
#define II_GAME_COMMON_VARIANT_SWITCH_H
#include <sfn/type_list.h>
#include <type_traits>
#include <variant>

namespace ii {
namespace detail {
template <typename>
struct variant_to_type_list;
template <typename... Args>
struct variant_to_type_list<std::variant<Args...>> {
  using type = sfn::list<Args...>;
};
}  // namespace detail

template <typename Variant>
using variant_to_type_list = typename detail::variant_to_type_list<Variant>::type;
template <typename Variant, typename T>
inline constexpr std::size_t variant_index = sfn::find<variant_to_type_list<Variant>, T>;

}  // namespace ii

#define VARIANT_CASE(T, variant, x)                              \
  case variant_index<std::remove_cvref_t<decltype(variant)>, T>: \
    if (auto& x = *std::get_if<T>(&(variant)); true)

#endif
