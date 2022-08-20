#ifndef II_GAME_LOGIC_SIM_IO_OUTPUT_H
#define II_GAME_LOGIC_SIM_IO_OUTPUT_H
#include "game/logic/sim/io/aggregate.h"
#include "game/logic/sim/io/events.h"
#include "game/logic/sim/io/render.h"
#include <bit>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {

struct aggregate_event {
  std::optional<background_fx_change> background_fx;
  std::vector<particle> particles;
  std::vector<sound_out> sounds;
  std::unordered_map<std::uint32_t, std::uint32_t> rumble_map;
  std::uint32_t global_rumble = 0;
};

struct aggregate_output {
  void clear() {
    entries.clear();
  }

  void append_to(aggregate_output& output) const {
    output.entries.insert(output.entries.end(), entries.begin(), entries.end());
  }

  struct entry {
    resolve_key key;
    aggregate_event e;
  };
  std::deque<entry> entries;
};

struct render_output {
  std::vector<render::player_info> players;
  std::vector<render::line_t> lines;
  std::uint64_t tick_count = 0;
  std::uint32_t lives_remaining = 0;
  std::optional<std::uint32_t> overmind_timer;
  std::optional<float> boss_hp_bar;
  std::uint32_t colour_cycle = 0;
};

struct sim_results {
  std::uint64_t tick_count = 0;
  std::uint32_t seed = 0;
  std::uint32_t lives_remaining = 0;

  struct player_result {
    std::uint32_t number = 0;
    std::uint64_t score = 0;
    std::uint32_t deaths = 0;
  };
  std::vector<player_result> players;
  std::vector<run_event> events;
  std::uint64_t score = 0;

  boss_flag bosses_killed() const {
    boss_flag flag{0};
    for (const auto& e : events) {
      if (e.boss_kill) {
        flag |= *e.boss_kill;
      }
    }
    return flag;
  }

  std::uint32_t boss_kill_count() const {
    return std::popcount(+bosses_killed());
  }
};

}  // namespace ii

#endif
