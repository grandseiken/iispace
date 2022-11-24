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

struct sound_out {
  sound sound_id{0};
  float volume = 0.f;
  float pan = 0.f;
  float pitch = 0.f;
};

struct rumble_out {
  std::uint32_t player_id = 0;
  std::uint32_t time_ticks = 0;
  float lf = 0.f;
  float hf = 0.f;
};

struct aggregate_event {
  std::optional<background_fx_change> background_fx;
  std::vector<particle> particles;
  std::vector<sound_out> sounds;
  std::vector<rumble_out> rumble;
};

struct aggregate_output {
  void clear() { entries.clear(); }
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
  render::background background;
  std::vector<render::player_info> players;
  std::vector<render::shape> shapes;
  std::uint64_t tick_count = 0;
  std::uint32_t lives_remaining = 0;
  std::optional<std::uint32_t> overmind_timer;
  std::optional<std::uint32_t> overmind_wave;
  std::optional<float> boss_hp_bar;
  std::uint32_t colour_cycle = 0;
  std::string debug_text;
};

struct transient_render_state {
  std::unordered_map<std::uint32_t, render::entity_state> entity_map;
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

  std::uint32_t boss_kill_count() const { return std::popcount(+bosses_killed()); }
};

}  // namespace ii

#endif
