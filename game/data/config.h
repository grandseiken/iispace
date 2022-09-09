
#ifndef II_GAME_DATA_CONFIG_H
#define II_GAME_DATA_CONFIG_H
#include "game/common/result.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii::data {

struct config {
  float volume = 1.f;
};

result<config> read_config(std::span<const std::uint8_t>);
result<std::vector<std::uint8_t>> write_config(const config&);

}  // namespace ii::data

#endif
