#ifndef II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#define II_GAME_LOGIC_SIM_SIM_INTERNALS_H
#include "game/common/random.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/collision.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/io/render.h"
#include "game/logic/sim/io/results.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {

struct SimInternals {
  SimInternals(std::uint32_t seed)
  : game_state_random{seed}, game_sequence_random{seed}, aesthetic_random{seed + 1} {}

  // Input.
  std::vector<input_frame> input_frames;
  RandomEngine game_state_random;
  RandomEngine game_sequence_random;
  RandomEngine aesthetic_random;

  // Internal sim data.
  initial_conditions conditions;
  ecs::EntityIndex index;
  ecs::entity_id global_entity_id{0};
  std::optional<ecs::handle> global_entity_handle;
  std::uint64_t tick_count = 0;
  std::unique_ptr<CollisionIndex> collision_index;

  // Run output.
  // TODO: if adding more, put in struct for easy copying in copy_to().
  boss_flag bosses_killed{0};

  // Per-frame output.
  std::optional<float> boss_hp_bar;
  std::vector<render_output::line_t> line_output;
  std::vector<render_output::player_info> player_output;
  aggregate_output output;
};

}  // namespace ii

#endif