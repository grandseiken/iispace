#include "game/logic/v0/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/biome.h"
#include "game/logic/v0/overmind/wave_data.h"

namespace ii::v0 {
namespace {

struct Overmind : ecs::component {
  std::size_t biome_index = 0;
  wave_data data;

  Overmind() {
    static constexpr std::int32_t kInitialPower = 32;
    data.power = kInitialPower;
  }

  void update(ecs::handle h, SimInterface& sim) {
    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });
    if (total_enemy_threat) {
      return;
    }

    auto& biomes = sim.conditions().biomes;
    if (biome_index >= biomes.size()) {
      return;
    }
    const auto* biome = get_biome(biomes[biome_index]);
    if (!biome) {
      return;
    }
    biome->spawn_wave(sim, data);

    if (data.wave_count < 5) {
      data.power += 2;
    }
    ++data.wave_count;
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