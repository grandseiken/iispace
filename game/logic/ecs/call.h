#ifndef II_GAME_LOGIC_ECS_CALL_H
#define II_GAME_LOGIC_ECS_CALL_H
#include "game/common/functional.h"
#include "game/logic/ecs/index.h"
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

template <typename T>
struct is_const_parameter
: std::bool_constant<(!is_component_parameter<T> || is_const_component_parameter<T>)&&(
      !is_handle_parameter<T> || is_const_handle_parameter<T>)> {};

template <typename T>
struct retain_parameter
: std::bool_constant<!is_component_parameter<T> && !is_handle_parameter<T>> {};

template <typename Arg, typename H>
constexpr bool can_synthesize_call_parameter(H&& handle) {
  if constexpr (is_handle_parameter<Arg> ||
                (is_component_parameter<Arg> && is_pointer_component_parameter<Arg>)) {
    return true;
  } else if (is_component_parameter<Arg>) {
    return handle.template has<std::remove_cvref_t<Arg>>();
  }
}

template <typename Arg, typename H>
constexpr decltype(auto) synthesize_call_parameter(H&& handle) {
  if constexpr (is_handle_parameter<Arg>) {
    return handle;
  } else if constexpr (is_component_parameter<Arg> && is_pointer_component_parameter<Arg>) {
    return handle.template get<std::remove_cvref_t<std::remove_pointer_t<Arg>>>();
  } else if (is_component_parameter<Arg>) {
    assert(handle.template has<std::remove_cvref_t<Arg>>());
    return *handle.template get<std::remove_cvref_t<Arg>>();
  }
}

template <bool Check, function auto F, typename H, typename... ForwardedArgs,
          typename... SynthesizedArgs>
consteval auto call_impl_create(tl::list<ForwardedArgs...>, tl::list<SynthesizedArgs...>) {
  return +[](H h, ForwardedArgs... args) {
    if constexpr (Check) {
      if (!(can_synthesize_call_parameter<SynthesizedArgs>(h) && ...)) {
        return;
      }
    }
    return F(synthesize_call_parameter<SynthesizedArgs>(h)..., args...);
  };
}

template <bool Check, function auto F>
struct call_impl {
  using parameters = parameter_types_of<decltype(F)>;
  static inline constexpr auto retain_index = tl::find_if<parameters, retain_parameter>;
  using forwarded_parameters = tl::sublist<parameters, retain_index>;
  using synthesized_parameters = tl::sublist<parameters, 0, retain_index>;
  using handle_type = std::conditional_t<tl::all_of<synthesized_parameters, is_const_parameter>,
                                         ecs::const_handle, ecs::handle>;
  static inline constexpr auto value = detail::call_impl_create<Check, F, handle_type>(
      forwarded_parameters{}, synthesized_parameters{});
};

template <typename T, typename F>
struct signature_impl;
template <typename T, typename R, typename... Args>
struct signature_impl<T, R (T::*)(Args...)> : std::type_identity<R(Args...)> {};
template <typename T, typename R, typename... Args>
struct signature_impl<T, R (T::*)(Args...) const> : std::type_identity<R(Args...)> {};

template <bool Check, bool Const, typename... SynthesizedArgs, typename F, typename... Args>
auto dispatch_invoke(tl::list<SynthesizedArgs...>, handle_base<Const> h, F&& f, Args&&... args) {
  if constexpr (Check) {
    if (!(can_synthesize_call_parameter<SynthesizedArgs>(h) && ...)) {
      return;
    }
  }
  return std::invoke(f, synthesize_call_parameter<SynthesizedArgs>(h)..., args...);
}

template <bool Check, bool Const, typename F, typename... Args>
auto dispatch_impl(handle_base<Const> h, F&& f, Args&&... args) {
  using f_type = std::remove_cvref_t<F>;
  using m_type = typename signature_impl<f_type, decltype(&f_type::operator())>::type;
  using parameters = parameter_types_of<m_type>;
  using synthesized_parameters =
      tl::sublist<parameters, 0, tl::find_if<parameters, retain_parameter>>;
  return dispatch_invoke<Check, Const>(synthesized_parameters{}, h, std::forward<F>(f),
                                       std::forward<Args>(args)...);
}

}  // namespace detail

template <function auto F>
inline constexpr auto call = detail::call_impl<false, unwrap<F>>::value;
template <function auto F>
inline constexpr auto call_if = detail::call_impl<true, unwrap<F>>::value;

// TODO: dispatch could use concept checking.
template <bool Const, typename F, typename... Args>
auto dispatch(handle_base<Const> h, F&& f, Args&&... args) {
  return detail::dispatch_impl<false>(h, std::forward<F>(f), std::forward<Args>(args)...);
}

template <bool Const, typename F, typename... Args>
auto dispatch_if(handle_base<Const> h, F&& f, Args&&... args) {
  return detail::dispatch_impl<true>(h, std::forward<F>(f), std::forward<Args>(args)...);
}

}  // namespace ii::ecs

#endif