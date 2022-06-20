#include "game/logic/boss/boss_internal.h"
#include "game/logic/ecs/call.h"
#include "game/logic/player/player.h"
#include <algorithm>

std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> Boss::fireworks_;
std::vector<vec2> Boss::warnings_;

namespace {
const fixed kHpPerExtraPlayer = 1_fx / 10;
const fixed kHpPerExtraCycle = 3 * 1_fx / 10;
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
  sim().index().iterate<ii::Enemy>([&](ii::ecs::handle h, const ii::Enemy&) {
    if (h.id() != handle().id()) {
      ii::ecs::call_if<&ii::Health::damage>(h, sim(), Player::kBombDamage, ii::damage_type::kBomb,
                                            std::nullopt);
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
    fireworks_.emplace_back(n, std::make_pair(shape().centre + v, shapes()[0]->colour));
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

namespace ii {
std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>>& boss_fireworks() {
  return ::Boss::fireworks_;
}

std::vector<vec2>& boss_warnings() {
  return ::Boss::warnings_;
}
}  // namespace ii