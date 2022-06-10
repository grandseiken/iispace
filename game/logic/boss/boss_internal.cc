#include "game/logic/boss/boss_internal.h"
#include "game/logic/player.h"
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

std::uint32_t scale_boss_damage(SimInterface& sim, ecs::handle, damage_type, std::uint32_t damage) {
  return damage * (60 / (1 + (sim.get_lives() ? sim.player_count() : sim.alive_players())));
}

std::function<void(damage_type)>
make_legacy_boss_on_hit(ecs::handle h, bool explode_on_bomb_damage) {
  auto boss = static_cast<::Boss*>(h.get<LegacyShip>()->ship.get());
  return [boss, explode_on_bomb_damage](damage_type type) {
    if (type == damage_type::kBomb && explode_on_bomb_damage) {
      boss->explosion();
      boss->explosion(glm::vec4{1.f}, 16);
      boss->explosion(std::nullopt, 24);
    }
  };
}

std::function<void(damage_type)> make_legacy_boss_on_destroy(ecs::handle h) {
  auto enemy = static_cast<::Enemy*>(h.get<LegacyShip>()->ship.get());
  return [enemy](damage_type type) {
    enemy->on_destroy(type == damage_type::kBomb);
  };
}

}  // namespace ii

Boss::Boss(ii::SimInterface& sim, const vec2& position)
: Enemy{sim, position, ii::ship_flag::kBoss} {}

void Boss::on_destroy(bool) {
  for (const auto& ship : sim().all_ships(ii::ship_flag::kEnemy)) {
    if (ship != this) {
      ship->damage(Player::kBombDamage, false, 0);
    }
  }
  explosion();
  explosion(glm::vec4{1.f}, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(glm::vec4{1.f}, 36);
  explosion(shapes()[0]->colour, 48);
  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < 16; ++i) {
    vec2 v = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{8 + sim().random(64) + sim().random(64)});
    fireworks_.push_back(
        std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
    n += i;
  }
  sim().rumble_all(25);
  play_sound(ii::sound::kExplosion);
}

namespace ii {
std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>>& boss_fireworks() {
  return ::Boss::fireworks_;
}

std::vector<vec2>& boss_warnings() {
  return ::Boss::warnings_;
}
}  // namespace ii