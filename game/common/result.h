#ifndef II_GAME_COMMON_RESULT_H
#define II_GAME_COMMON_RESULT_H
#include <tl/expected.hpp>
#include <string>

namespace ii {

template <typename T>
struct [[nodiscard]] result : tl::expected<T, std::string> {
  using expected::expected;
};

inline auto unexpected(const std::string& s) -> decltype(tl::unexpected(s)) {
  return tl::unexpected(s);
}

inline auto unexpected(std::string&& s) -> decltype(tl::unexpected(s)) {
  return tl::unexpected(s);
}

}  // namespace ii

#endif