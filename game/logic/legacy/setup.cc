#include "game/logic/legacy/setup.h"
#include "game/logic/legacy/overmind/overmind.h"
#include "game/logic/legacy/player/player.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/io/events.h"
#include "game/logic/sim/sim_interface.h"

namespace ii::legacy {

auto LegacySimSetup::parameters(const initial_conditions& conditions) const -> game_parameters {
  game_parameters result;
  result.fps = conditions.mode == game_mode::kLegacy_Fast ? 60 : 50;
  result.dimensions = {640, 480};
  return result;
}

void LegacySimSetup::initialise_systems(SimInterface& sim) {
  sim.index().on_component_add<Destroy>([&sim](ecs::handle h, const Destroy& d) {
    auto* e = h.get<Enemy>();
    if (e && e->score_reward && d.source && d.destroy_type != damage_type::kBomb) {
      if (auto* p = sim.index().get<Player>(*d.source); p) {
        p->add_score(sim, e->score_reward);
      }
    }
    if (e && e->boss_score_reward) {
      sim.index().iterate<Player>([&](Player& p) {
        if (!p.is_killed()) {
          p.add_score(sim, e->boss_score_reward / sim.alive_players());
        }
      });
    }
    if (auto* b = h.get<Boss>(); b) {
      sim.trigger(run_event::boss_kill_event(b->boss));
    }
  });
}

ecs::entity_id LegacySimSetup::start_game(const initial_conditions& conditions,
                                          std::span<const std::uint32_t> ai_players,
                                          SimInterface& sim) {
  auto dim = sim.dimensions();
  auto lives = conditions.mode == game_mode::kLegacy_Boss
      ? conditions.player_count * GlobalData::kBossModeLives
      : GlobalData::kStartingLives;

  auto global_entity = sim.index().create(GlobalData{.lives = lives});
  global_entity.add(Update{.update = ecs::call<&GlobalData::pre_update>});
  global_entity.add(PostUpdate{.post_update = ecs::call<&GlobalData::post_update>});

  legacy::spawn_overmind(sim);
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v{(1 + i) * dim.x.to_int() / (1 + conditions.player_count), dim.y / 2};
    legacy::spawn_player(
        sim, v, i,
        /* AI */ std::find(ai_players.begin(), ai_players.end(), i) != ai_players.end());
  }

  return global_entity.id();
}

std::optional<input_frame> LegacySimSetup::ai_think(const SimInterface& sim, ecs::handle h) {
  return legacy::ai_think(sim, h);
}

}  // namespace ii::legacy