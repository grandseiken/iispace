#ifndef II_GAME_LOGIC_SIM_IO_RESULTS_H
#define II_GAME_LOGIC_SIM_IO_RESULTS_H
#include "game/logic/sim/io/result_events.h"
#include <cstdint>
#include <vector>

namespace ii {

struct sim_results {
  std::uint64_t tick_count = 0;
  std::uint32_t seed = 0;
  std::uint32_t lives_remaining = 0;
  boss_flag bosses_killed{0};

  struct player_result {
    std::uint32_t number = 0;
    std::uint64_t score = 0;
    std::uint32_t deaths = 0;
  };
  std::vector<player_result> players;
  std::uint64_t score = 0;
};

}  // namespace ii

#endif
