#ifndef II_GAME_COMMON_STRUCT_TUPLE_H
#define II_GAME_COMMON_STRUCT_TUPLE_H
#include <tuple>
#include <type_traits>

#define FE_PARENS ()
#define FE_EXPAND0(...) FE_EXPAND1(FE_EXPAND1(FE_EXPAND1(FE_EXPAND1(__VA_ARGS__))))
#define FE_EXPAND1(...) FE_EXPAND2(FE_EXPAND2(FE_EXPAND2(FE_EXPAND2(__VA_ARGS__))))
#define FE_EXPAND2(...) FE_EXPAND3(FE_EXPAND3(FE_EXPAND3(FE_EXPAND3(__VA_ARGS__))))
#define FE_EXPAND3(...) FE_EXPAND4(FE_EXPAND4(FE_EXPAND4(FE_EXPAND4(__VA_ARGS__))))
#define FE_EXPAND4(...) __VA_ARGS__

#define FOR_EACH(macro, ...) __VA_OPT__(FE_EXPAND0(FE_IMPL(macro, __VA_ARGS__)))
#define FE_IMPL(macro, a1, ...) macro(a1) __VA_OPT__(, FE_AGAIN FE_PARENS(macro, __VA_ARGS__))
#define FE_AGAIN() FE_IMPL

template <typename T>
struct struct_tuple_member_entry {
  const char* name;
  T value;
};
template <typename T>
inline auto make_struct_tuple_member_entry(const char* name, T&& t) {
  return struct_tuple_member_entry<std::decay_t<T>>{name, std::forward<T>(t)};
}

#define DEBUG_STRUCT_TUPLE_MEMBER(member) ::make_struct_tuple_member_entry(#member, x.member)
#define DEBUG_STRUCT_TUPLE(struct_name, ...)                                    \
  inline const char* to_debug_name(const struct_name*) { return #struct_name; } \
  inline auto to_debug_tuple(const struct_name& x) {                            \
    (void)x;                                                                    \
    return std::tuple{FOR_EACH(DEBUG_STRUCT_TUPLE_MEMBER, __VA_ARGS__)};        \
  }

#endif
