#ifndef II_GAME_DATA_PACKET_H
#define II_GAME_DATA_PACKET_H
#include "game/logic/sim/sim_io.h"
#include <cstdint>
#include <optional>

namespace ii {

struct sim_packet {
  // Tick count at which the input frames apply.
  std::uint64_t tick_count = 0;
  std::vector<input_frame> input_frames;
  // Tick count at which canonical state is known, and optional checksum thereof.
  std::uint64_t canonical_tick_count = 0;
  std::uint32_t canonical_checksum = 0;  // Zero to skip verification.
};

}  // namespace ii

#endif
