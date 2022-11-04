#include "game/logic/v0/setup.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/overmind/overmind.h"
#include "game/logic/v0/player/player.h"
#include "game/logic/v0/player/powerup.h"

namespace ii::v0 {
namespace {

void run_drop(std::int32_t& counter, std::uint32_t chance, SimInterface& sim, const vec2& position,
              sfn::ptr<void(SimInterface&, const vec2&)> spawn) {
  static constexpr std::uint32_t kProbability = 100;
  auto drop = [&] { spawn(sim, position); };
  auto& r = sim.random(random_source::kGameSequence);

  for (std::uint32_t i = 0; i < chance; ++i) {
    if (counter >= 0 && (counter >= kProbability || !r.uint(kProbability))) {
      spawn(sim, position);
      if (counter >= kProbability) {
        counter -= 2 * static_cast<std::int32_t>(kProbability);
      } else {
        counter = 0;
      }
    }
    ++counter;
  }
}

}  // namespace

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
  v0::spawn_overmind(sim);

  sim.index().on_component_add<Destroy>([&sim](ecs::handle h, const Destroy&) {
    auto& data = *sim.global_entity().get<GlobalData>();
    auto* table = h.get<DropTable>();
    auto* transform = h.get<Transform>();
    if (table && transform) {
      run_drop(data.shield_drop_counter, table->shield_drop_chance, sim, transform->centre,
               &spawn_shield_powerup);
      run_drop(data.bomb_drop_counter, table->bomb_drop_chance, sim, transform->centre,
               &spawn_bomb_powerup);
    }
  });

  auto dim = sim.dimensions();
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v{(1 + i) * dim.x.to_int() / (1 + conditions.player_count), dim.y / 2};
    v0::spawn_player(sim, v, i);
  }
  return global_entity.id();
}

}  // namespace ii::v0