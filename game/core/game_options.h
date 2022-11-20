#ifndef II_GAME_CORE_GAME_OPTIONS_H
#define II_GAME_CORE_GAME_OPTIONS_H
#include "game/logic/sim/io/conditions.h"
#include <cstdint>
#include <vector>

namespace ii {

struct game_options_t {
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::vector<std::uint32_t> ai_players;
  std::vector<std::uint32_t> replay_remote_players;
  std::uint64_t replay_min_tick_delivery_delay = 0;
  std::uint64_t replay_max_tick_delivery_delay = 0;

  // Should probably be in-game options.
  bool windowed = false;
  bool debug = false;
};

}  // namespace ii

#endif