#include "game/logic/v0/setup.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/overmind.h"
#include "game/logic/v0/player/player.h"

namespace ii::v0 {

auto V0SimSetup::parameters(const initial_conditions&) const -> game_parameters {
  game_parameters result;
  result.fps = 60;
  result.dimensions = {960, 540};
  return result;
}

void V0SimSetup::initialise_systems(SimInterface& sim) {}

ecs::entity_id V0SimSetup::start_game(const initial_conditions& conditions,
                                      std::span<const std::uint32_t> ai_players,
                                      SimInterface& sim) {
  auto global_entity = sim.index().create();
  v0::spawn_overmind(sim);

  auto dim = sim.dimensions();
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v{(1 + i) * dim.x.to_int() / (1 + conditions.player_count), dim.y / 2};
    v0::spawn_player(
        sim, v, i,
        /* AI */ std::find(ai_players.begin(), ai_players.end(), i) != ai_players.end());
  }
  return global_entity.id();
}

std::optional<input_frame> V0SimSetup::ai_think(const SimInterface& sim, ecs::handle h) {
  return v0::ai_think(sim, h);
}

}  // namespace ii::v0