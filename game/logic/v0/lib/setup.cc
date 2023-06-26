#include "game/logic/v0/lib/setup.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/overmind/overmind.h"
#include "game/logic/v0/player/player.h"
#include "game/logic/v0/player/powerup.h"
#include "game/logic/v0/player/shot.h"

namespace ii::v0 {
namespace {

void run_drop(drop_data& data, bool& dropped, std::uint32_t chance, SimInterface& sim,
              const vec2& position, sfn::ptr<void(SimInterface&, const vec2&)> spawn) {
  static constexpr std::uint32_t kProbability = 1000;
  static constexpr auto kIProbability = static_cast<std::int32_t>(kProbability);
  auto& r = sim.random(random_source::kGameSequence);
  auto i_chance = static_cast<std::int32_t>(chance);

  data.counter += i_chance;
  if (dropped) {
    return;
  }
  data.compensation += i_chance;
  if (data.counter >= 0 && chance > 0 &&
      (data.counter >= kIProbability || r.uint(kProbability) < chance)) {
    spawn(sim, position);
    if (data.counter >= kIProbability) {
      data.counter -= kIProbability + data.compensation;
    } else {
      data.counter -= data.compensation;
    }
    data.compensation = 0;
    dropped = true;
  }
}

}  // namespace

auto V0SimSetup::parameters(const initial_conditions&) const -> game_parameters {
  game_parameters result;
  result.fps = 60;
  result.dimensions = {960, 540};
  return result;
}

void V0SimSetup::initialise_systems(SimInterface& sim) {
  sim.index().on_component_add<Destroy>([&sim](ecs::handle h, const Destroy&) {
    auto& data = *sim.global_entity().get<GlobalData>();
    const auto* table = h.get<DropTable>();
    const auto* transform = h.get<Transform>();
    if (table && transform) {
      bool dropped = false;
      run_drop(data.shield_drop, dropped, table->shield_drop_chance, sim, transform->centre,
               &spawn_shield_powerup);
      run_drop(data.bomb_drop, dropped, table->bomb_drop_chance, sim, transform->centre,
               &spawn_bomb_powerup);
    }
  });
}

ecs::entity_id V0SimSetup::start_game(const initial_conditions& conditions, SimInterface& sim) {
  auto global_entity = sim.index().create(GlobalData{});

  v0::spawn_overmind(sim);
  auto dim = sim.dimensions();
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v{(1 + i) * dim.x.to_int() / (1 + conditions.player_count), dim.y / 2};
    v0::spawn_player(sim, v, i);
  }
  return global_entity.id();
}

void V0SimSetup::begin_tick(SimInterface& sim) {
  sim.index().iterate_dispatch_if<Physics>(
      [](Physics& physics, Transform& transform) { physics.update(transform); });
  sim.index().iterate_dispatch<EnemyStatus>(
      [&](ecs::handle h, EnemyStatus& status, Update* update) { status.update(h, update, sim); });
}

bool V0SimSetup::is_game_over(const SimInterface& sim) const {
  const auto& global_data = sim.global_entity().get<GlobalData>();
  return global_data->is_run_complete ||
      (sim.killed_players() == sim.player_count() && !count_player_shots(sim));
}

}  // namespace ii::v0
