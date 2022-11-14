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
  static constexpr std::uint32_t kInitialPower = 16;
  static constexpr std::uint32_t kSpawnTimer = 60;

  std::size_t biome_index = 0;
  std::uint32_t spawn_timer = 0;
  wave_data data;

  Overmind() { data.power = kInitialPower; }

  void update(ecs::handle h, SimInterface& sim) {
    if (!sim.tick_count()) {
      background_fx_change change;
      change.type = background_fx_type::kBiome0;
      sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kBackgroundFx)).background_fx(change);
    }

    if (spawn_timer) {
      // TODO: clarify how the rest timer interacts with wall despawns exactly...
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
    if (total_enemy_threat <= data.threat_trigger) {
      spawn_timer = kSpawnTimer;
    }
  }

  void spawn_wave(SimInterface& sim) {
    auto& global = *sim.global_entity().get<GlobalData>();
    auto wave = static_cast<std::int32_t>(data.wave_count);
    auto count = static_cast<std::int32_t>(sim.player_count());
    global.shield_drop.counter += 120 + (6 * wave / (2 + count)) + 80 * count;
    global.bomb_drop.counter += 160 + (9 * wave / (2 + count)) + 60 * count;
    if (!data.wave_count) {
      global.bomb_drop.counter += 200;
    }

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
    if (!data.wave_count) {
      data.power += 3 * (sim.player_count() - 1);
    }
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