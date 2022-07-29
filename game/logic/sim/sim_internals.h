#ifndef II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#define II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/fx/stars.h"
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

  SimInternals(const SimInternals& other)
  : random_engine{other.random_engine}
  , conditions{other.conditions}
  , global_entity_id{other.global_entity_id}
  , tick_count{other.tick_count}
  , particles{other.particles}
  , stars{other.stars}
  , collisions{other.collisions}
  , bosses_killed{other.bosses_killed} {
    other.index.copy_to(index);
  }

  SimInternals& operator=(const SimInternals&) = delete;
  // Input.
  std::vector<input_frame>* input_frames = nullptr;
  RandomEngine random_engine;

  // Internal sim data.
  initial_conditions conditions;
  ecs::EntityIndex index;
  ecs::entity_id global_entity_id{0};
  std::optional<ecs::handle> global_entity_handle;
  std::uint64_t tick_count = 0;
  std::vector<particle> particles;
  Stars stars;

  struct collision_entry {
    ecs::entity_id id;
    ecs::handle handle;
    const Transform* transform = nullptr;
    const Collision* collision = nullptr;
    fixed x_min = 0;
  };
  std::vector<collision_entry> collisions;

  // Run output.
  // TODO: if adding more, put in struct for easy copying in copy_to().
  boss_flag bosses_killed{0};

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
  std::unordered_map<sound, sound_aggregation_t> sound_output;
};

}  // namespace ii

#endif