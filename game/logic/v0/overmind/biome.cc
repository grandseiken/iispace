#include "game/logic/v0/overmind/biome.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/formation_set.h"
#include "game/logic/v0/overmind/formations/formations.h"
#include "game/logic/v0/overmind/spawn_context.h"

namespace ii::v0 {
namespace {

wave_data get_wave_data(const initial_conditions& conditions, const wave_id& wave) {
  static constexpr std::uint32_t kInitialPower = 16u;

  wave_data data;
  data.power = (wave.biome_index + 1) * kInitialPower + 2u * (conditions.player_count - 1u);
  data.power += wave.wave_number;
  data.power += std::min(8u, wave.wave_number);
  data.upgrade_budget = data.power / 2;
  data.threat_trigger = wave.wave_number;
  return data;
}

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
  }

  bool update_background(RandomEngine& random, const background_input& input,
                         background_update& data) const override {
    if (input.initialise) {
      data.position_delta = {0.f, 0.f, 1.f / 8, 1.f / 256};
      data.rotation_delta = 0.f;

      data.type = render::background::type::kBiome0;
      data.colour = colour::kSolarizedDarkBase03;
      data.colour.z /= 1.25f;
      data.parameters = {0.f, 0.f};
    }

    if (input.wave_number) {
      if (*input.wave_number % 2) {
        data.parameters.x = 1.f - data.parameters.x;
      } else {
        auto v = from_polar(2 * fixed_c::pi * random.fixed(), 1_fx / 2);
        data.position_delta.x = v.x.to_float();
        data.position_delta.y = v.y.to_float();
      }
      return true;
    }
    return false;
  }

  wave_data
  get_wave_data(const initial_conditions& conditions, const wave_id& wave) const override {
    return v0::get_wave_data(conditions, wave);
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