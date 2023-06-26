#include "game/logic/v0/overmind/overmind.h"
#include "game/common/easing.h"
#include "game/common/math.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/overmind/biome.h"
#include "game/logic/v0/overmind/wave_data.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/player.h"
#include "game/logic/v0/player/upgrade.h"
#include "game/render/data/background.h"
#include <array>

namespace ii::v0 {
namespace {

struct Overmind : ecs::component {
  static constexpr std::uint32_t kSpawnTimer = 60;
  static constexpr std::uint32_t kSpawnTimerBoss = 300;
  static constexpr std::uint32_t kSpawnTimerUpgrade = 180;
  static constexpr std::uint32_t kBackgroundInterpolateTime = 180;
  static constexpr std::array kBossProgressRespawn = {
      75_fx / 100, 50_fx / 100, 35_fx / 100, 25_fx / 100, 175_fx / 1000, 10_fx / 100, 5_fx / 100};

  wave_id next_wave;
  std::uint32_t spawn_timer = 0;
  std::optional<wave_data> current_wave;
  std::vector<wave_data> wave_list;
  std::vector<render::background::update> background_updates;
  std::uint32_t background_interpolate = 0;
  std::optional<fixed> boss_progress;

  void update(ecs::handle h, SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    global.walls_vulnerable = !(sim.index().count<Enemy>() - sim.index().count<WallTag>());

    std::optional<background_input> input;

    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });
    if (spawn_timer) {
      global.walls_vulnerable = false;
      if (!--spawn_timer) {
        respawn_players(sim);
        spawn_wave(sim);
      }
    } else if (auto t = init_spawn_wave(sim, total_enemy_threat); t) {
      input.emplace();
      input->biome_index = current_wave->biome_index;
      input->type = current_wave->type;
      input->wave_number = next_wave.wave_number;
      spawn_timer = *t;
      cleanup_mod_upgrades(sim);
    }

    if (current_wave->type == wave_type::kBoss) {
      if (!boss_progress) {
        boss_progress = 1_fx;
      }
      std::uint32_t boss_hp = 0;
      std::uint32_t boss_max_hp = 0;
      sim.index().iterate_dispatch_if<Boss>([&](const Boss& boss, const Health& health) {
        boss_hp += health.hp;
        boss_max_hp += health.max_hp;
      });
      auto progress = fixed{boss_hp} / boss_max_hp;
      for (auto t : kBossProgressRespawn) {
        if (progress <= t && t < *boss_progress) {
          respawn_players(sim);
          break;
        }
      }
      boss_progress = progress;
    } else {
      boss_progress.reset();
    }

    if (const auto* biome = get_biome(sim, current_wave->biome_index); biome) {
      update_background(sim, *biome, input, sim.global_entity().get<Background>()->background);
    }

    global.overmind_wave_count = next_wave.wave_number;
    global.debug_text.clear();
    global.debug_text += "s:" + std::to_string(global.shield_drop.counter) +
        " b:" + std::to_string(global.bomb_drop.counter);
    global.debug_text += " t:" + std::to_string(total_enemy_threat);
  }

  std::optional<std::uint32_t>
  init_spawn_wave(const SimInterface& sim, std::uint32_t total_enemy_threat) {
    if (next_wave.wave_number >= wave_list.size()) {
      current_wave && ++next_wave.biome_index;
      next_wave.wave_number = 0;
      if (const auto* biome = get_biome(sim, next_wave.biome_index); biome) {
        wave_list = biome->get_wave_list(sim.conditions(), next_wave.biome_index);
        for (auto& wave : wave_list) {
          wave.biome_index = next_wave.biome_index;
        }
      } else {
        wave_list = {wave_data{.type = wave_type::kRunComplete}};
      }
    }

    if (next_wave.wave_number >= wave_list.size()) {
      return false;
    }
    auto next_data = wave_list[next_wave.wave_number];
    auto next_timer = next_data.type == wave_type::kBoss ? kSpawnTimerBoss : 0u;
    bool next_condition = next_data.type == wave_type::kEnemy || !total_enemy_threat;
    bool prev_condition = false;

    std::uint32_t prev_timer = kSpawnTimer;
    if (!current_wave) {
      prev_condition = true;
    } else {
      switch (current_wave->type) {
      case wave_type::kEnemy:
        prev_condition = total_enemy_threat <= current_wave->threat_trigger;
        prev_timer = kSpawnTimer;
        break;

      case wave_type::kBoss:
        prev_condition = !total_enemy_threat;
        prev_timer = kSpawnTimerBoss;
        break;

      case wave_type::kUpgrade:
        prev_condition = is_mod_upgrade_choice_done(sim);
        prev_timer = kSpawnTimerUpgrade;
        break;

      case wave_type::kRunComplete:
        prev_condition = true;
        break;
      }
    }
    if (prev_condition && next_condition) {
      current_wave = next_data;
      return std::max(prev_timer, next_timer);
    }
    return std::nullopt;
  }

  void spawn_wave(SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    auto n = static_cast<std::int32_t>(8u * next_wave.biome_index + next_wave.wave_number);
    auto c = static_cast<std::int32_t>(sim.player_count());

    const auto* biome = get_biome(sim, next_wave.biome_index);
    const auto& data = wave_list[next_wave.wave_number];
    if (data.type == wave_type::kEnemy) {
      global.shield_drop.counter += 120 + (6 * n) / (2 + c) + 90 * c;
      global.bomb_drop.counter += 160 + (9 * n) / (2 + c) + 80 * c;
      if (!next_wave.biome_index && !next_wave.wave_number) {
        global.bomb_drop.counter += 200;
      }
    }

    switch (data.type) {
    case wave_type::kEnemy:
      if (biome) {
        biome->spawn_wave(sim, data);
      }
      break;

    case wave_type::kBoss:
      if (biome) {
        biome->spawn_boss(sim, data.biome_index);
      }
      break;

    case wave_type::kUpgrade:
      spawn_upgrades(sim);
      break;

    case wave_type::kRunComplete:
      global.is_run_complete = true;
      break;
    }
    ++next_wave.wave_number;
  }

  void spawn_upgrades(SimInterface& sim) const {
    player_loadout combined_loadout;
    sim.index().iterate<PlayerLoadout>([&](const PlayerLoadout& loadout) {
      for (const auto& pair : loadout.loadout) {
        combined_loadout[pair.first] += pair.second;
      }
    });
    auto mods =
        mod_selection(sim.conditions(), sim.random(random_source::kGameSequence), combined_loadout);
    spawn_mod_upgrades(sim, mods);
  }

  void update_background(SimInterface& sim, const Biome& biome,
                         const std::optional<background_input>& input,
                         render::background& background) {
    // TODO: finish moving all this out to reuse seamless background transitions in menus.
    if (background_updates.empty()) {
      background.position.x = sim.random_fixed().to_float() * 1024.f - 512.f;
      background.position.y = sim.random_fixed().to_float() * 1024.f - 512.f;
    }
    if (input) {
      background_updates.emplace_back(
          biome.update_background(sim.random(random_source::kAesthetic), *input));
      if (background_updates.size() == 1u) {
        background_updates.back().fill_defaults();
      } else {
        background_updates.back().fill_from(*(background_updates.rbegin() + 1));
      }
    }

    if (background_updates.size() > 1 && ++background_interpolate == kBackgroundInterpolateTime) {
      background_updates.erase(background_updates.begin());
      background_interpolate = 0;
    }

    auto set_data = [&](render::background::data& data, const render::background::update& update) {
      data.type = *update.type;
      data.parameters = *update.parameters;
      data.colour = *update.colour;
    };

    render::background::update u0;
    if (background_updates.empty()) {
      u0.fill_defaults();
    } else {
      u0 = background_updates[0];
    }
    set_data(background.data0, background_updates[0]);

    background.interpolate =
        ease_in_out_cubic(static_cast<float>(background_interpolate) / kBackgroundInterpolateTime);
    if (background_updates.size() > 1) {
      set_data(background.data1, background_updates[1]);
      background.position +=
          glm::mix(*u0.velocity, *background_updates[1].velocity, background.interpolate);
      background.rotation += glm::mix(*u0.angular_velocity, *background_updates[1].angular_velocity,
                                      background.interpolate);
    } else {
      background.position += *u0.velocity;
      background.rotation += *u0.angular_velocity;
    }
    background.rotation = normalise_angle(background.rotation);
  }

  const Biome* get_biome(const SimInterface& sim, std::uint32_t index) const {
    const auto& biomes = sim.conditions().biomes;
    return index >= biomes.size() ? nullptr : v0::get_biome(biomes[index]);
  }
};
DEBUG_STRUCT_TUPLE(Overmind, next_wave, spawn_timer, current_wave, wave_list,
                   background_interpolate);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  add(h, Update{.update = ecs::call<&Overmind::update>});
}

}  // namespace ii::v0
