#ifndef II_GAME_DATA_CRYPT_H
#define II_GAME_DATA_CRYPT_H
#include "game/common/result.h"
#include <nonstd/span.hpp>
#include <cstdint>
#include <vector>

namespace ii {

std::vector<std::uint8_t>
crypt(nonstd::span<const std::uint8_t> text, nonstd::span<const std::uint8_t> key);
ii::result<std::vector<std::uint8_t>> compress(nonstd::span<const std::uint8_t> bytes);
ii::result<std::vector<std::uint8_t>> decompress(nonstd::span<const std::uint8_t> bytes);

}  // namespace ii

#endif
