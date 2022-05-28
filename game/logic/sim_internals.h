#ifndef IISPACE_GAME_LOGIC_SIM_INTERNALS_H
#define IISPACE_GAME_LOGIC_SIM_INTERNALS_H
#include "game/common/z.h"
#include "game/logic/ship.h"
#include "game/logic/sim_interface.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class Overmind;

namespace ii {

struct SimInternals {
  // Internal sim data.
  game_mode mode = game_mode::kNormal;
  using ship_list = std::vector<Ship*>;
  std::int32_t lives = 0;
  std::vector<std::unique_ptr<Ship>> ships;
  std::vector<Particle> particles;
  ship_list player_list;
  ship_list collisions;
  // TODO: try to remove. Only needed for get_non_wall_count(). Which is a weird function.
  Overmind* overmind = nullptr;

  // Per-frame output.
  std::optional<float> boss_hp_bar;

  // Run output.
  std::int32_t bosses_killed = 0;
  std::int32_t hard_mode_bosses_killed = 0;
};

}  // namespace ii

#endif