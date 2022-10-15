#include "game/logic/v0/setup.h"
#include "game/logic/ship/components.h"
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

ecs::entity_id V0SimSetup::start_game(const initial_conditions& conditions, SimInterface& sim) {
  // TODO: not all strictly necessary. Should divide global data up into legacy/v0 components.
  auto global_entity = sim.index().create(GlobalData{});
  global_entity.add(Update{.update = ecs::call<&GlobalData::pre_update>});
  global_entity.add(PostUpdate{.post_update = ecs::call<&GlobalData::post_update>});
  v0::spawn_overmind(sim);

  auto dim = sim.dimensions();
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v{(1 + i) * dim.x.to_int() / (1 + conditions.player_count), dim.y / 2};
    v0::spawn_player(sim, v, i);
  }
  return global_entity.id();
}

}  // namespace ii::v0