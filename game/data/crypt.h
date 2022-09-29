#ifndef II_GAME_DATA_CRYPT_H
#define II_GAME_DATA_CRYPT_H
#include "game/common/result.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii::data {

std::vector<std::uint8_t>
crypt(std::span<const std::uint8_t> text, std::span<const std::uint8_t> key);
ii::result<std::vector<std::uint8_t>> compress(std::span<const std::uint8_t> bytes);
ii::result<std::vector<std::uint8_t>> decompress(std::span<const std::uint8_t> bytes);

}  // namespace ii::data

#endif
