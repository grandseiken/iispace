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

  wave_id wave;
  std::uint32_t threat_trigger = 0;
  std::uint32_t spawn_timer = 0;

  void update(ecs::handle h, SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    global.walls_vulnerable = !(sim.index().count<Enemy>() - sim.index().count<WallTag>());
    global.debug_text.clear();
    global.debug_text += "s:" + std::to_string(global.shield_drop.counter) +
        " b:" + std::to_string(global.bomb_drop.counter);

    auto& background = sim.global_entity().get<Background>()->background;
    // TODO: delegate to biome somehow.
    auto t = static_cast<float>(sim.tick_count());
    background.interpolate = 0.f;
    background.position = {256.f * std::cos(t / 512.f), -t / 4.f, t / 8.f, t / 256.f};
    background.rotation = 0.f;
    background.data0.type = render::background::type::kBiome0;
    background.data0.colour = colour::kSolarizedDarkBase03;
    background.data0.colour.z /= 1.25f;
    background.data0.parameters = {(1.f - std::cos(t / 1024.f)) / 2.f, 0.f};

    if (spawn_timer) {
      global.walls_vulnerable = false;
      // TODO: bosses. Legacy behaviour was 20/24/28/32 waves per boss by player count.
      // Not sure if we should preserve that or just stick with increasing wave power.
      if (!--spawn_timer) {
        respawn_players(sim);
        spawn_wave(sim);
      }
      return;
    }

    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });
    if (total_enemy_threat <= threat_trigger) {
      spawn_timer = kSpawnTimer;
    }
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

  const Biome* get_biome(const SimInterface& sim) const {
    const auto& biomes = sim.conditions().biomes;
    return wave.biome_index >= biomes.size() ? nullptr : v0::get_biome(biomes[wave.biome_index]);
  }
};
DEBUG_STRUCT_TUPLE(Overmind, wave.biome_index, wave.wave_number, threat_trigger, spawn_timer);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  h.emplace<Update>().update = ecs::call<&Overmind::update>;
}

}  // namespace ii::v0