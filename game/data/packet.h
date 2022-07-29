#ifndef II_GAME_DATA_PACKET_H
#define II_GAME_DATA_PACKET_H
#include "game/logic/sim/sim_io.h"
#include <cstdint>
#include <optional>

namespace ii {

struct sim_packet {
  std::uint64_t tick_count = 0;
  std::optional<uint32_t> checksum;
  std::vector<input_frame> input_frames;
};

}  // namespace ii

#endif
