#ifndef IISPACE_GAME_LOGIC_SIM_SIM_INTERNALS_H
#define IISPACE_GAME_LOGIC_SIM_SIM_INTERNALS_H
#include "game/common/z.h"
#include "game/logic/ship.h"
#include "game/logic/sim/input_adapter.h"
#include "game/logic/sim/sim_interface.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Overmind;

namespace ii {

struct SimInternals {
  // Input.
  std::vector<input_frame> input_frames;

  // Internal sim data.
  game_mode mode = game_mode::kNormal;
  using ship_list = std::vector<Ship*>;
  std::int32_t lives = 0;
  std::vector<std::unique_ptr<Ship>> ships;
  std::vector<particle> particles;
  ship_list player_list;
  ship_list collisions;
  // TODO: try to remove. Only needed for get_non_wall_count(). Which is a weird function.
  Overmind* overmind = nullptr;

  // Per-frame output.
  struct line_output_t {
    fvec2 a;
    fvec2 b;
    colour_t c = 0;
  };
  struct player_info_t {
    colour_t colour = 0;
    std::int64_t score = 0;
    std::int32_t multiplier = 0;
    float timer = 0.f;
  };
  struct sound_aggregation_t {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };
  std::optional<float> boss_hp_bar;
  std::vector<line_output_t> line_output;
  std::vector<player_info_t> player_output;
  std::unordered_map<std::int32_t, std::int32_t> rumble_output;
  std::unordered_map<ii::sound, sound_aggregation_t> sound_output;

  // Run output.
  std::int32_t bosses_killed = 0;
  std::int32_t hard_mode_bosses_killed = 0;
};

}  // namespace ii

#endif