#ifndef II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#define II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#include "game/logic/ship/ship.h"
#include "game/logic/sim/random_engine.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_io.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Overmind;

namespace ii {

struct SimInternals {
  SimInternals(std::uint32_t seed) : random_engine{seed} {}
  // Input.
  std::vector<input_frame> input_frames;
  RandomEngine random_engine;

  // Internal sim data.
  game_mode mode = game_mode::kNormal;
  using ship_list = std::vector<Ship*>;
  std::uint32_t lives = 0;
  std::vector<std::unique_ptr<Ship>> ships;
  std::vector<particle> particles;
  ship_list player_list;
  ship_list collisions;
  // TODO: try to remove. Only needed for get_non_wall_count(). Which is a weird function.
  Overmind* overmind = nullptr;

  // Per-frame output.
  struct sound_aggregation_t {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };
  std::optional<float> boss_hp_bar;
  std::vector<render_output::line_t> line_output;
  std::vector<render_output::player_info> player_output;
  std::unordered_map<std::uint32_t, std::uint32_t> rumble_output;
  std::unordered_map<ii::sound, sound_aggregation_t> sound_output;

  // Run output.
  std::uint32_t bosses_killed = 0;
  std::uint32_t hard_mode_bosses_killed = 0;
};

}  // namespace ii

#endif