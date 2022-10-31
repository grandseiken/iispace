#ifndef II_GAME_LOGIC_LEGACY_COMPONENTS_H
#define II_GAME_LOGIC_LEGACY_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/components.h"

namespace ii::legacy {

struct PlayerScore : ecs::component {
  std::uint64_t score = 0;
  std::uint32_t multiplier = 1;
  std::uint32_t multiplier_count = 0;
  void add(SimInterface&, std::uint64_t s);
};
DEBUG_STRUCT_TUPLE(PlayerScore, score, multiplier, multiplier_count);

struct GlobalData : ecs::component {
  static constexpr std::uint32_t kBombDamage = 50;
  static constexpr std::uint32_t kStartingLives = 2;
  static constexpr std::uint32_t kBossModeLives = 1;

  std::uint32_t lives = 0;
  std::uint32_t non_wall_enemy_count = 0;
  std::optional<std::uint32_t> overmind_wave_timer;
  std::vector<std::uint32_t> player_kill_queue;

  struct fireworks_entry {
    std::uint32_t time = 0;
    vec2 position{0};
    glm::vec4 colour{0.f};
  };
  std::vector<fireworks_entry> fireworks;
  std::vector<vec2> extra_enemy_warnings;

  void pre_update(SimInterface&);                // Runs before any other entity updates.
  void post_update(ecs::handle, SimInterface&);  // Runs after any other entity updates.
};
DEBUG_STRUCT_TUPLE(GlobalData, lives, non_wall_enemy_count, overmind_wave_timer, player_kill_queue);

}  // namespace ii::legacy

#endif
