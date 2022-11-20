#include "game/logic/v0/overmind/biome.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/formation_set.h"
#include "game/logic/v0/overmind/formations/formations.h"
#include "game/logic/v0/overmind/spawn_context.h"

namespace ii::v0 {
namespace {
spawn_context make_context(SimInterface& sim, const wave_data& wave) {
  return {sim, sim.random(random_source::kGameSequence), wave, 0};
}

void spawn(spawn_context& context) {
  // Shuffle.
  for (std::uint32_t i = 0; i + 1 < context.output.entries.size(); ++i) {
    auto j = i + context.random.uint(context.output.entries.size() - i);
    std::swap(context.output.entries[i], context.output.entries[j]);
  }

  // Spawn.
  context.upgrade_budget = context.data.upgrade_budget;
  for (const auto& e : context.output.entries) {
    e.function(context, e.side, e.position);
  }
}

class TestingBiome : public Biome {
public:
  ~TestingBiome() override = default;
  TestingBiome() {
    // TODO: 1 more mixed hub/sponge pattern?
    auto s = set.add_set();
    s.add<formations::follow0>();
    s.add<formations::follow1>();
    s.add<formations::follow2>();
    s.add<formations::follow3>();
    s.add<formations::follow4>();
    s.add<formations::chaser0>();
    s.add<formations::chaser1>();
    s.add<formations::chaser2>();
    s.add<formations::chaser3>();
    s.add<formations::follow0_side>();
    s.add<formations::follow1_side>();
    s.add<formations::follow2_side>();
    s.add<formations::follow3_side>();
    s.add<formations::chaser0_side>();
    s.add<formations::chaser1_side>();
    s.add<formations::chaser2_side>();
    s.add<formations::chaser3_side>();

    s.add<formations::hub0>();
    s.add<formations::hub1>();
    s.add<formations::hub0_side>();
    s.add<formations::hub1_side>();

    s.add<formations::square0>();
    s.add<formations::square1>();
    s.add<formations::square2>();
    s.add<formations::square0_side>();
    s.add<formations::square1_side>();
    s.add<formations::square2_side>();
    s.add<formations::square3_side>();

    s.add<formations::wall0>();
    s.add<formations::wall1>();
    s.add<formations::wall2>();
    s.add<formations::wall0_side>();
    s.add<formations::wall1_side>();
    s.add<formations::wall2_side>();
    s.add<formations::wall3_side>();

    s.add<formations::shielder0>();
    s.add<formations::shielder0_side>();
    s.add<formations::sponge0>();
    s.add<formations::sponge0_side>();

    s.add<formations::tractor0>();
    s.add<formations::tractor1>();
    s.add<formations::tractor0_side>();
    s.add<formations::tractor1_side>();
    s.add<formations::shield_hub0_side>();

    s.add<formations::mixed0>();
    s.add<formations::mixed1>();
    s.add<formations::mixed2>();
    s.add<formations::mixed3>();
    s.add<formations::mixed4>();
    s.add<formations::mixed5>();
    s.add<formations::mixed6>();
    s.add<formations::mixed0_side>();
    s.add<formations::mixed1_side>();
    s.add<formations::mixed2_side>();
    s.add<formations::mixed3_side>();
    s.add<formations::mixed4_side>();
    s.add<formations::mixed5_side>();
    s.add<formations::mixed6_side>();
    s.add<formations::mixed7_side>();
    s.add<formations::mixed8_side>();
    s.add<formations::mixed9_side>();
    s.add<formations::mixed10_side>();
  }

  void spawn_wave(SimInterface& sim, const wave_data& wave) const override {
    auto context = make_context(sim, wave);
    set.spawn_wave(context);
    spawn(context);
  }

private:
  FormationSet set;
};

}  // namespace

const Biome* get_biome(run_biome biome) {
  static TestingBiome testing_biome;
  switch (biome) {
  case run_biome::kTesting:
    return &testing_biome;
  }
  return nullptr;
}

}  // namespace ii::v0