#include "game/logic/v0/overmind/biome.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/boss/boss.h"
#include "game/logic/v0/overmind/formation_set.h"
#include "game/logic/v0/overmind/formations/biome0.h"
#include "game/logic/v0/overmind/spawn_context.h"

namespace ii::v0 {
namespace {

wave_data get_enemy_wave_data(const initial_conditions& conditions, std::uint32_t biome_index,
                              std::uint32_t phase_index, std::uint32_t wave_number) {
  static constexpr std::uint32_t kBiomePower = 16u;
  static constexpr std::uint32_t kPhasePower = 2u;
  static constexpr std::uint32_t kPlayerPower = 4u;
  static constexpr std::uint32_t kPlayerBiomePower = 2u;
  static constexpr std::uint32_t kBiomeThreat = 8u;
  static constexpr std::uint32_t kPhaseThreat = 4u;

  wave_data data;
  data.type = wave_type::kEnemy;
  data.biome_index = biome_index;
  data.power = (biome_index + 1u) * kBiomePower;
  data.power += (conditions.player_count - 1u) * kPlayerPower;
  data.power += biome_index * (2u * conditions.player_count - 1u) * kPlayerBiomePower;
  data.power += wave_number + std::min(10u, wave_number);
  data.power += phase_index * phase_index * kPhasePower;
  data.upgrade_budget = data.power / 2;
  data.threat_trigger = wave_number;
  data.threat_trigger += biome_index * kBiomeThreat;
  data.threat_trigger += phase_index * kPhaseThreat;
  return data;
}

std::vector<wave_data>
get_wave_list(const initial_conditions& conditions, std::uint32_t biome_index) {
  std::vector<wave_data> result;
  std::uint32_t enemy_wave_number = 0;
  for (std::uint32_t i = 0u; i < 8u; ++i) {
    result.emplace_back(
        get_enemy_wave_data(conditions, biome_index, /* phase */ 0, enemy_wave_number++));
  }
  result.emplace_back(wave_data{wave_type::kUpgrade});
  for (std::uint32_t i = 0u; i < 7u + (conditions.player_count - 1u) / 2u; ++i) {
    result.emplace_back(
        get_enemy_wave_data(conditions, biome_index, /* phase */ 1, enemy_wave_number++));
  }
  result.emplace_back(wave_data{wave_type::kUpgrade});
  for (std::uint32_t i = 0u; i < 3u + conditions.player_count / 2u; ++i) {
    result.emplace_back(
        get_enemy_wave_data(conditions, biome_index, /* phase */ 2, enemy_wave_number++));
  }
  result.emplace_back(wave_data{wave_type::kBoss});
  result.emplace_back(wave_data{wave_type::kUpgrade});
  return result;
}

spawn_context make_context(SimInterface& sim, const wave_data& wave) {
  return {sim, sim.random(random_source::kGameSequence), wave, 0};
}

void spawn(spawn_context& context) {
  context.random.shuffle(context.output.entries.begin(), context.output.entries.end());
  context.upgrade_budget = context.data.upgrade_budget;
  for (const auto& e : context.output.entries) {
    e.function(context, e.side, e.position);
  }
}

class TestingBiome : public Biome {
public:
  ~TestingBiome() override = default;

  render::background::update
  update_background(RandomEngine& random, const background_input& input) const override {
    render::background::update update;
    update.type = render::background::type::kBiome0;
    update.colour = colour::solarized::kDarkBase03;
    update.colour->z /= 1.25f;
    update.parameters = {0.f, 0.f};

    auto c = input.wave_number % 8;
    update.type =
        c < 4 ? render::background::type::kBiome0 : render::background::type::kBiome0_Polar;
    update.parameters->x = c >= 2 && c < 6 ? 1.f : 0.f;
    if (!input.wave_number || c % 2) {
      auto v = from_polar(2 * pi<fixed> * random.fixed(), 1_fx / 2);
      update.velocity = {v.x.to_float(), v.y.to_float(), 1.f / 8, 1.f / 2};
      update.angular_velocity = 0.f;
    }

    if (input.type == wave_type::kBoss) {
      auto v = from_polar(2 * pi<fixed> * random.fixed(), 2_fx);
      update.velocity = {v.x.to_float(), v.y.to_float(), 1.f / 8, 1.f / 2};
      update.colour = colour::solarized::kDarkBase03;
      update.colour->y /= 2.f;
      update.colour->z /= 1.375f;
    }
    return update;
  }

  std::vector<wave_data>
  get_wave_list(const initial_conditions& conditions, std::uint32_t biome_index) const override {
    std::vector<wave_data> result;
    result.emplace_back(wave_data{wave_type::kUpgrade});
    result.emplace_back(wave_data{wave_type::kUpgrade});
    result.emplace_back(wave_data{wave_type::kBoss});
    return result;
  }

  void spawn_wave(SimInterface&, const wave_data&) const override {}

  void spawn_boss(SimInterface& sim, std::uint32_t biome_index) const override {
    spawn_biome0_square_boss(sim, biome_index);
  }
};

class Biome0_Uplink : public Biome {
public:
  ~Biome0_Uplink() override = default;
  Biome0_Uplink() {
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

  render::background::update
  update_background(RandomEngine& random, const background_input& input) const override {
    render::background::update update;
    update.type = render::background::type::kBiome0;
    update.colour = colour::solarized::kDarkBase03;
    update.colour->z /= 1.25f;
    update.parameters = {0.f, 0.f};

    auto c = input.wave_number % 8;
    update.type =
        c < 4 ? render::background::type::kBiome0 : render::background::type::kBiome0_Polar;
    update.parameters->x = c >= 2 && c < 6 ? 1.f : 0.f;
    if (!input.wave_number || c % 2) {
      auto v = from_polar(2 * pi<fixed> * random.fixed(), 1_fx / 2);
      update.velocity = {v.x.to_float(), v.y.to_float(), 1.f / 8, 1.f / 2};
      update.angular_velocity = 0.f;
    }

    if (input.type == wave_type::kBoss) {
      auto v = from_polar(2 * pi<fixed> * random.fixed(), 2_fx);
      update.velocity = {v.x.to_float(), v.y.to_float(), 1.f / 8, 1.f / 2};
      update.colour = colour::solarized::kDarkBase03;
      update.colour->y /= 2.f;
      update.colour->z /= 1.375f;
    }
    return update;
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

  void spawn_boss(SimInterface& sim, std::uint32_t biome_index) const override {
    spawn_biome0_square_boss(sim, biome_index);
  }

private:
  FormationSet set;
};

}  // namespace

const Biome* get_biome(run_biome biome) {
  static TestingBiome testing;
  static Biome0_Uplink biome0;
  switch (biome) {
  case run_biome::kTesting:
    return &testing;
  case run_biome::kBiome0_Uplink:
  case run_biome::kBiome1_Edge:
  case run_biome::kBiome2_Fabric:
  case run_biome::kBiome3_Archive:
  case run_biome::kBiome4_Firewall:
  case run_biome::kBiome5_Prism:
  case run_biome::kBiome6_SystemCore:
  case run_biome::kBiome7_DarkNet:
  case run_biome::kBiome8_Paradise:
  case run_biome::kBiome9_Minus:
    return &biome0;
  }
  return nullptr;
}

}  // namespace ii::v0