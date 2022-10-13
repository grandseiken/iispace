#ifndef II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#define II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#include <cstdint>

namespace ii::v0 {

struct wave_data {
  std::uint32_t wave_count = 0;
  std::uint32_t power = 0;
};

}  // namespace ii::v0

#endif
