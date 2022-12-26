#include "game/logic/v0/overmind/biome.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/formation_set.h"
#include "game/logic/v0/overmind/formations/biome0.h"
#include "game/logic/v0/overmind/spawn_context.h"

namespace ii::v0 {
namespace {

wave_data get_enemy_wave_data(const initial_conditions& conditions, std::uint32_t biome_index,
                              bool second_half, std::uint32_t wave_number) {
  static constexpr std::uint32_t kInitialPower = 16u;

  wave_data data;
  data.type = wave_type::kEnemy;
  data.biome_index = biome_index;
  data.power = (biome_index + 1) * kInitialPower + 2u * (conditions.player_count - 1u);
  data.power += wave_number;
  data.power += std::min(10u, wave_number);
  data.power += second_half ? 4u : 0u;
  data.upgrade_budget = data.power / 2;
  data.threat_trigger = wave_number;
  data.threat_trigger += second_half ? 4u : 0u;
  return data;
}

std::vector<wave_data>
get_wave_list(const initial_conditions& conditions, std::uint32_t biome_index) {
  std::vector<wave_data> result;
  std::uint32_t enemy_wave_number = 0;
  for (std::uint32_t i = 0u; i < 10u; ++i) {
    result.emplace_back(get_enemy_wave_data(conditions, biome_index, false, enemy_wave_number++));
  }
  result.emplace_back(wave_data{wave_type::kUpgrade});
  for (std::uint32_t i = 0u; i < 11u + conditions.player_count; ++i) {
    result.emplace_back(get_enemy_wave_data(conditions, biome_index, true, enemy_wave_number++));
  }
  result.emplace_back(wave_data{wave_type::kBoss});
  result.emplace_back(wave_data{wave_type::kUpgrade});
  return result;
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
    auto s = set.add_set();
    s.add<formations::biome0::follow0>();
    s.add<formations::biome0::follow1>();
    s.add<formations::biome0::follow2>();
    s.add<formations::biome0::follow3>();
    s.add<formations::biome0::follow4>();
    s.add<formations::biome0::chaser0>();
    s.add<formations::biome0::chaser1>();
    s.add<formations::biome0::chaser2>();
    s.add<formations::biome0::chaser3>();
    s.add<formations::biome0::follow0_side>();
    s.add<formations::biome0::follow1_side>();
    s.add<formations::biome0::follow2_side>();
    s.add<formations::biome0::follow3_side>();
    s.add<formations::biome0::chaser0_side>();
    s.add<formations::biome0::chaser1_side>();
    s.add<formations::biome0::chaser2_side>();
    s.add<formations::biome0::chaser3_side>();

    s.add<formations::biome0::hub0>();
    s.add<formations::biome0::hub1>();
    s.add<formations::biome0::hub2>();
    s.add<formations::biome0::hub0_side>();
    s.add<formations::biome0::hub1_side>();

    s.add<formations::biome0::square0>();
    s.add<formations::biome0::square1>();
    s.add<formations::biome0::square2>();
    s.add<formations::biome0::square0_side>();
    s.add<formations::biome0::square1_side>();
    s.add<formations::biome0::square2_side>();
    s.add<formations::biome0::square3_side>();

    s.add<formations::biome0::wall0>();
    s.add<formations::biome0::wall1>();
    s.add<formations::biome0::wall2>();
    s.add<formations::biome0::wall0_side>();
    s.add<formations::biome0::wall1_side>();
    s.add<formations::biome0::wall2_side>();
    s.add<formations::biome0::wall3_side>();

    s.add<formations::biome0::shielder0>();
    s.add<formations::biome0::shielder0_side>();
    s.add<formations::biome0::sponge0>();
    s.add<formations::biome0::sponge0_side>();

    s.add<formations::biome0::tractor0>(2);
    s.add<formations::biome0::tractor1>();
    s.add<formations::biome0::tractor0_side>();
    s.add<formations::biome0::tractor1_side>();
    s.add<formations::biome0::shield_hub0_side>(2);

    s.add<formations::biome0::mixed0>();
    s.add<formations::biome0::mixed1>();
    s.add<formations::biome0::mixed2>();
    s.add<formations::biome0::mixed3>();
    s.add<formations::biome0::mixed4>();
    s.add<formations::biome0::mixed5>();
    s.add<formations::biome0::mixed6>();
    s.add<formations::biome0::mixed7>();
    s.add<formations::biome0::mixed8>();
    s.add<formations::biome0::mixed0_side>();
    s.add<formations::biome0::mixed1_side>();
    s.add<formations::biome0::mixed2_side>();
    s.add<formations::biome0::mixed3_side>();
    s.add<formations::biome0::mixed4_side>();
    s.add<formations::biome0::mixed5_side>();
    s.add<formations::biome0::mixed6_side>();
    s.add<formations::biome0::mixed7_side>();
    s.add<formations::biome0::mixed8_side>();
    s.add<formations::biome0::mixed9_side>();
    s.add<formations::biome0::mixed10_side>();
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
      if (*input.wave_number % 4 == 3) {
        data.type = data.type == render::background::type::kBiome0
            ? render::background::type::kBiome0_Polar
            : render::background::type::kBiome0;
      } else if (*input.wave_number % 4 == 1) {
        data.parameters.x = 1.f - data.parameters.x;
      } else {
        auto v = from_polar(2 * pi<fixed> * random.fixed(), 1_fx / 2);
        data.position_delta.x = v.x.to_float();
        data.position_delta.y = v.y.to_float();
      }
      return true;
    }
    return false;
  }

  std::vector<wave_data>
  get_wave_list(const initial_conditions& conditions, std::uint32_t biome_index) const override {
    return v0::get_wave_list(conditions, biome_index);
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