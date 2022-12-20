#include "game/logic/v0/overmind/overmind.h"
#include "game/common/easing.h"
#include "game/common/math.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/overmind/biome.h"
#include "game/logic/v0/overmind/wave_data.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/player.h"
#include "game/logic/v0/player/upgrade.h"

namespace ii::v0 {
namespace {

struct Overmind : ecs::component {
  static constexpr std::uint32_t kSpawnTimer = 60;
  static constexpr std::uint32_t kSpawnTimerBoss = 180;
  static constexpr std::uint32_t kSpawnTimerUpgrade = 120;
  static constexpr std::uint32_t kBackgroundInterpolateTime = 180;

  wave_id next_wave;
  std::uint32_t spawn_timer = 0;
  std::optional<wave_data> current_wave;
  std::vector<wave_data> wave_list;
  std::vector<background_update> background_data;
  std::uint32_t background_interpolate = 0;

  void update(ecs::handle h, SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    global.walls_vulnerable = !(sim.index().count<Enemy>() - sim.index().count<WallTag>());

    background_input input;
    input.tick_count = sim.tick_count();

    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });
    if (spawn_timer) {
      global.walls_vulnerable = false;
      // TODO: bosses. Legacy behaviour was 20/24/28/32 waves per boss by player count.
      // Not sure if we should preserve that or just stick with increasing wave power.
      if (!--spawn_timer) {
        respawn_players(sim);
        spawn_wave(sim);
      }
    } else if (auto t = init_spawn_wave(sim, total_enemy_threat); t) {
      input.initialise = !next_wave.wave_number;
      input.wave_number = next_wave.wave_number;
      spawn_timer = *t;
      cleanup_mod_upgrades(sim);
    }
    input.biome_index = current_wave->biome_index;

    if (const auto* biome = get_biome(sim, current_wave->biome_index); biome) {
      update_background(sim, *biome, input, sim.global_entity().get<Background>()->background);
    }

    global.debug_text.clear();
    global.debug_text += "s:" + std::to_string(global.shield_drop.counter) +
        " b:" + std::to_string(global.bomb_drop.counter);
    global.debug_text += " t:" + std::to_string(total_enemy_threat);
    global.overmind_wave_count = next_wave.wave_number;
  }

  std::optional<std::uint32_t>
  init_spawn_wave(const SimInterface& sim, std::uint32_t total_enemy_threat) {
    if (next_wave.wave_number >= wave_list.size()) {
      current_wave && ++next_wave.biome_index;
      next_wave.wave_number = 0;
      if (const auto* biome = get_biome(sim, next_wave.biome_index); biome) {
        wave_list = biome->get_wave_list(sim.conditions(), next_wave.biome_index);
      }
    }
    if (next_wave.wave_number >= wave_list.size()) {
      return false;
    }

    auto next_data = wave_list[next_wave.wave_number];
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
      }
    }
    if (prev_condition && next_condition) {
      current_wave = next_data;
      return prev_timer;
    }
    return std::nullopt;
  }

  void spawn_wave(SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    auto n = static_cast<std::int32_t>(8u * next_wave.biome_index + next_wave.wave_number);
    auto c = static_cast<std::int32_t>(sim.player_count());
    global.shield_drop.counter += 120 + (6 * n / (2 + c)) + 80 * c;
    global.bomb_drop.counter += 160 + (9 * n / (2 + c)) + 60 * c;
    if (!next_wave.biome_index && !next_wave.wave_number) {
      global.bomb_drop.counter += 200;
    }

    if (const auto* biome = get_biome(sim, next_wave.biome_index); biome) {
      const auto& data = wave_list[next_wave.wave_number];
      switch (data.type) {
      case wave_type::kEnemy:
        biome->spawn_wave(sim, data);
        break;

      case wave_type::kBoss:
        break;

      case wave_type::kUpgrade:
        spawn_upgrades(sim);
        break;
      }
    }
    ++next_wave.wave_number;
  }

  void spawn_upgrades(SimInterface& sim) const {
    std::vector<std::span<const mod_id>> current_loadouts;
    sim.index().iterate<PlayerLoadout>(
        [&](const PlayerLoadout& loadout) { current_loadouts.emplace_back(loadout.loadout); });
    auto mods =
        mod_selection(sim.conditions(), sim.random(random_source::kGameSequence), current_loadouts);
    spawn_mod_upgrades(sim, mods);
  }

  void update_background(SimInterface& sim, const Biome& biome, const background_input& input,
                         render::background& background) {
    bool transition = true;
    if (background_data.empty()) {
      background.position.x = sim.random_fixed().to_float() * 1024.f - 512.f;
      background.position.y = sim.random_fixed().to_float() * 1024.f - 512.f;
      background_data.emplace_back();
      transition = false;
    }
    auto update_copy = background_data.back();
    if (!biome.update_background(sim.random(random_source::kAesthetic), input, update_copy)) {
      transition = false;
    }
    if (transition) {
      background_data.emplace_back(update_copy);
    } else {
      background_data.back() = update_copy;
    }

    if (background_data.size() > 1 && ++background_interpolate == kBackgroundInterpolateTime) {
      background_data.erase(background_data.begin());
      background_interpolate = 0;
    }

    auto set_data = [](render::background::data& data, const background_update& update) {
      data.type = update.type;
      data.colour = update.colour;
      data.parameters = update.parameters;
    };

    background.interpolate =
        ease_in_out_cubic(static_cast<float>(background_interpolate) / kBackgroundInterpolateTime);
    set_data(background.data0, background_data[0]);
    if (background_data.size() > 1) {
      set_data(background.data1, background_data[1]);
      background.position += glm::mix(background_data[0].position_delta,
                                      background_data[1].position_delta, background.interpolate);
      background.rotation += glm::mix(background_data[0].rotation_delta,
                                      background_data[1].rotation_delta, background.interpolate);
    } else {
      background.position += background_data[0].position_delta;
      background.rotation += background_data[0].rotation_delta;
    }
    background.rotation = normalise_angle(background.rotation);
  }

  const Biome* get_biome(const SimInterface& sim, std::uint32_t index) const {
    const auto& biomes = sim.conditions().biomes;
    return biomes.empty() ? nullptr : v0::get_biome(biomes[index % biomes.size()]);
  }
};
DEBUG_STRUCT_TUPLE(Overmind, next_wave.biome_index, next_wave.wave_number, spawn_timer,
                   background_interpolate);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  h.emplace<Update>().update = ecs::call<&Overmind::update>;
}

}  // namespace ii::v0