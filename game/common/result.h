#ifndef II_GAME_COMMON_RESULT_H
#define II_GAME_COMMON_RESULT_H
#include <expected>
#include <string>

namespace ii {

template <typename T>
using result = std::expected<T, std::string>;
using std::unexpected;

}  // namespace ii

#endif
