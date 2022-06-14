#ifndef II_GAME_LOGIC_SHIP_ECS_CALL_H
#define II_GAME_LOGIC_SHIP_ECS_CALL_H
#include "game/common/functional.h"
#include "game/logic/ship/ecs_index.h"
#include <cassert>
#include <type_traits>

namespace ii::ecs {
namespace detail {

template <typename T>
inline constexpr bool is_handle_parameter = std::is_same_v<std::remove_cvref_t<T>, handle> ||
    std::is_same_v<std::remove_cvref_t<T>, const_handle>;
template <typename T>
inline constexpr bool is_const_handle_parameter =
    std::is_same_v<std::remove_cvref_t<T>, const_handle>;
template <typename T>
inline constexpr bool is_component_parameter =
    Component<std::remove_cvref_t<std::remove_pointer_t<T>>>;
template <typename T>
inline constexpr bool is_const_component_parameter =
    std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>;
template <typename T>
inline constexpr bool is_pointer_component_parameter = std::is_pointer_v<T>;

template <typename... Args>
inline constexpr bool is_const_handle_parameter_list =
    (((!is_component_parameter<Args> || is_const_component_parameter<Args>)&&(
         !is_handle_parameter<Args> || is_const_handle_parameter<Args>)) &&
     ...);
template <typename... Args>
inline constexpr bool has_handle_parameter = (is_handle_parameter<Args> || ...);

template <typename T>
struct convert_parameter_predicate
: std::bool_constant<is_component_parameter<T> || is_handle_parameter<T>> {};
template <typename>
struct convert_parameter_list;
template <typename... Args>
struct convert_parameter_list<type_list<Args...>>
: std::type_identity<type_list_drop_front<type_list<Args...>, convert_parameter_predicate>> {};

template <typename>
struct get_handle_parameter;
template <typename... Args>
struct get_handle_parameter<type_list<Args...>>
: std::type_identity<
      std::conditional_t<is_const_handle_parameter_list<Args...>, ecs::const_handle, ecs::handle>> {
};

template <typename Arg, typename H>
constexpr bool can_synthesize_call_parameter(H handle) {
  if constexpr (is_handle_parameter<Arg> ||
                (is_component_parameter<Arg> && is_pointer_component_parameter<Arg>)) {
    return true;
  } else if (is_component_parameter<Arg>) {
    return handle.template has<std::remove_cvref_t<Arg>>();
  }
}

template <typename Arg, typename H>
constexpr auto&& synthesize_call_parameter(H handle) {
  if constexpr (is_handle_parameter<Arg>) {
    return handle;
  } else if constexpr (is_component_parameter<Arg> && is_pointer_component_parameter<Arg>) {
    return handle.template get<std::remove_cvref_t<std::remove_pointer_t<Arg>>>();
  } else if (is_component_parameter<Arg>) {
    assert(handle.template has<std::remove_cvref_t<Arg>>());
    return *handle.template get<std::remove_cvref_t<Arg>>();
  }
}

template <bool, function_type, typename, typename, function auto>
inline constexpr auto call_impl_invoke = nullptr;

template <bool Check, typename R, typename... ConvertedArgs, typename H,
          typename... SynthesizedArgs, function auto F>
inline constexpr auto
    call_impl_invoke<Check, R(ConvertedArgs...), H, type_list<SynthesizedArgs...>, F> =
        +[](H h, ConvertedArgs... args) -> R {
  if constexpr (Check) {
    if (!(can_synthesize_call_parameter<SynthesizedArgs>(h) && ...)) {
      return;
    }
  }
  return F(synthesize_call_parameter<SynthesizedArgs>(h)..., args...);
};

template <bool Check, function auto F>
struct call_impl {
  using return_type = return_type_t<decltype(F)>;
  using parameter_types = parameter_types_t<decltype(F)>;
  using converted_parameter_list = typename detail::convert_parameter_list<parameter_types>::type;
  using synthesized_parameter_list =
      type_list_first_n<parameter_types,
                        type_list_size<parameter_types> - type_list_size<converted_parameter_list>>;
  using handle_type = typename detail::get_handle_parameter<parameter_types>::type;
  static inline constexpr auto value =
      detail::call_impl_invoke<Check, make_function_type<return_type, converted_parameter_list>,
                               handle_type, synthesized_parameter_list, F>;
};

}  // namespace detail

template <function auto F>
inline constexpr auto call = detail::call_impl<false, unwrap<F>>::value;
template <function auto F>
inline constexpr auto call_if = detail::call_impl<true, unwrap<F>>::value;

}  // namespace ii::ecs

#endif