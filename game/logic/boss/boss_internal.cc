#include "game/logic/boss/boss_internal.h"
#include "game/logic/ecs/call.h"
#include "game/logic/player/player.h"
#include <algorithm>

namespace {
constexpr fixed kHpPerExtraPlayer = 1_fx / 10;
constexpr fixed kHpPerExtraCycle = 3 * 1_fx / 10;
}  // namespace

namespace ii {

std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle) {
  std::uint32_t s =
      5000 * (cycle + 1) + 2500 * (boss > boss_flag::kBoss1C) + 2500 * (boss > boss_flag::kBoss2C);
  std::uint32_t r = s;
  for (std::uint32_t i = 0; i < players - 1; ++i) {
    r += (s * kHpPerExtraPlayer).to_int();
  }
  return r;
}

std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle) {
  auto hp = base * 30;
  auto r = hp;
  for (std::uint32_t i = 0; i < cycle; ++i) {
    r += (hp * kHpPerExtraCycle).to_int();
  }
  for (std::uint32_t i = 0; i < players - 1; ++i) {
    r += (hp * kHpPerExtraPlayer).to_int();
  }
  for (std::uint32_t i = 0; i < cycle * (players - 1); ++i) {
    r += (hp * kHpPerExtraCycle + kHpPerExtraPlayer).to_int();
  }
  return r;
}

std::uint32_t scale_boss_damage(ecs::handle, SimInterface& sim, damage_type, std::uint32_t damage) {
  return damage * (60 / (1 + (sim.get_lives() ? sim.player_count() : sim.alive_players())));
}

}  // namespace ii

Boss::Boss(ii::SimInterface& sim, const vec2& position) : ii::Ship{sim, position} {}

void Boss::on_destroy(bool) {
  sim().index().iterate_dispatch_if<ii::Enemy>([&](ii::ecs::handle h, ii::Health& health) {
    if (h.id() != handle().id()) {
      health.damage(h, sim(), ii::Player::kBombDamage, ii::damage_type::kBomb, std::nullopt);
    }
  });
  explosion();
  explosion(glm::vec4{1.f}, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(glm::vec4{1.f}, 36);
  explosion(shapes()[0]->colour, 48);
  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < 16; ++i) {
    vec2 v = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{8 + sim().random(64) + sim().random(64)});
    sim().global_entity().get<ii::GlobalData>()->fireworks.push_back(
        ii::GlobalData::fireworks_entry{
            .time = n, .position = shape().centre + v, .colour = shapes()[0]->colour});
    n += i;
  }
  sim().rumble_all(25);
  play_sound(ii::sound::kExplosion);
}

void Boss::render() const {
  if (auto* c = handle().get<ii::Health>(); c && c->hit_timer) {
    for (std::size_t i = 0; i < shapes().size(); ++i) {
      bool hit_flash = !c->hit_flash_ignore_index || i < *c->hit_flash_ignore_index;
      shapes()[i]->render(sim(), to_float(shape().centre), shape().rotation().to_float(),
                          hit_flash ? std::make_optional(glm::vec4{1.f}) : std::nullopt);
    }
    return;
  }
  Ship::render();
}
