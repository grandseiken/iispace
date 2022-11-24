#include "game/logic/v0/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/overmind/biome.h"
#include "game/logic/v0/overmind/wave_data.h"
#include "game/logic/v0/player/player.h"

namespace ii::v0 {
namespace {

struct Overmind : ecs::component {
  static constexpr std::uint32_t kSpawnTimer = 60;
  static constexpr std::uint32_t kBackgroundInterpolateTime = 120;

  wave_id wave;
  std::uint32_t threat_trigger = 0;
  std::uint32_t spawn_timer = 0;

  std::uint32_t background_interpolate = 0;
  std::vector<background_update> background_data;

  void update(ecs::handle h, SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    global.walls_vulnerable = !(sim.index().count<Enemy>() - sim.index().count<WallTag>());

    background_input input;
    input.initialise = !sim.tick_count();
    input.tick_count = sim.tick_count();
    input.biome_index = wave.biome_index;

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
    } else if (!input.initialise && total_enemy_threat <= threat_trigger) {
      spawn_timer = kSpawnTimer;
      input.wave_number = wave.wave_number;
    }

    if (const auto* biome = get_biome(sim); biome) {
      update_background(sim, *biome, input, sim.global_entity().get<Background>()->background);
    }

    global.debug_text.clear();
    global.debug_text += "s:" + std::to_string(global.shield_drop.counter) +
        " b:" + std::to_string(global.bomb_drop.counter);
    global.debug_text += " t:" + std::to_string(total_enemy_threat);
  }

  void spawn_wave(SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    auto n = static_cast<std::int32_t>(wave.wave_number);
    auto c = static_cast<std::int32_t>(sim.player_count());
    global.shield_drop.counter += 120 + (6 * n / (2 + c)) + 80 * c;
    global.bomb_drop.counter += 160 + (9 * n / (2 + c)) + 60 * c;
    if (!wave.wave_number) {
      global.bomb_drop.counter += 200;
    }

    if (const auto* biome = get_biome(sim); biome) {
      spawn_wave(sim, *biome);
    }
  }

  void spawn_wave(SimInterface& sim, const Biome& biome) {
    auto data = biome.get_wave_data(sim.conditions(), wave);
    biome.spawn_wave(sim, data);
    threat_trigger = data.threat_trigger;
    ++wave.wave_number;
    sim.global_entity().get<GlobalData>()->overmind_wave_count = wave.wave_number;
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
        static_cast<float>(background_interpolate) / kBackgroundInterpolateTime;
    set_data(background.data0, background_data[0]);
    if (background_data.size() > 1) {
      set_data(background.data1, background_data[1]);
      background.position += glm::mix(background_data[0].position_delta,
                                      background_data[1].position_delta, background.interpolate);
      background.rotation += glm::mix(background_data[0].rotation_delta,
                                      background_data[1].rotation_delta, background.interpolate);
    } else {
      background.position += background_data[0].position_delta;
      background.rotation += background_data[1].rotation_delta;
    }
    background.rotation = normalise_angle(background.rotation);
  }

  const Biome* get_biome(const SimInterface& sim) const {
    const auto& biomes = sim.conditions().biomes;
    return wave.biome_index >= biomes.size() ? nullptr : v0::get_biome(biomes[wave.biome_index]);
  }
};
DEBUG_STRUCT_TUPLE(Overmind, wave.biome_index, wave.wave_number, threat_trigger, spawn_timer,
                   background_interpolate);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  h.emplace<Update>().update = ecs::call<&Overmind::update>;
}

}  // namespace ii::v0