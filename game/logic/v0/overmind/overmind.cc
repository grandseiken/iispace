#include "game/logic/v0/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/biome.h"
#include "game/logic/v0/overmind/wave_data.h"
#include "game/logic/v0/player/player.h"

namespace ii::v0 {
namespace {

struct Overmind : ecs::component {
  static constexpr std::uint32_t kInitialPower = 16;
  static constexpr std::uint32_t kSpawnTimer = 60;

  std::size_t biome_index = 0;
  std::uint32_t spawn_timer = 0;
  wave_data data;

  Overmind() { data.power = kInitialPower; }

  void update(SimInterface& sim) {
    if (spawn_timer) {
      if (!--spawn_timer) {
        spawn_wave(sim);
      }
      return;
    }

    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });
    if (total_enemy_threat <= data.threat_trigger) {
      respawn_players(sim);
      spawn_timer = kSpawnTimer;
    }
  }

  void spawn_wave(SimInterface& sim) {
    const auto& biomes = sim.conditions().biomes;
    if (biome_index >= biomes.size()) {
      return;
    }
    const auto* biome = get_biome(biomes[biome_index]);
    if (!biome) {
      return;
    }
    spawn_wave(sim, *biome);
  }

  void spawn_wave(SimInterface& sim, const Biome& biome) {
    biome.spawn_wave(sim, data);
    if (data.wave_count < 5) {
      data.power += 2;
    } else {
      ++data.power;
    }
    data.upgrade_budget = data.power / 2;
    ++data.threat_trigger;
    ++data.wave_count;
    sim.global_entity().get<GlobalData>()->overmind_wave_count = data.wave_count;
  }
};
DEBUG_STRUCT_TUPLE(Overmind, biome_index, data.wave_count, data.power);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  h.emplace<Update>().update = ecs::call<&Overmind::update>;
}

}  // namespace ii::v0