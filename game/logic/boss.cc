#include "game/logic/boss.h"
#include "game/logic/player.h"
#include <algorithm>

std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> Boss::fireworks_;
std::vector<vec2> Boss::warnings_;

namespace {
const fixed kHpPerExtraPlayer = 1_fx / 10;
const fixed kHpPerExtraCycle = 3 * 1_fx / 10;
}  // namespace

Boss::Boss(ii::SimInterface& sim, const vec2& position, ii::SimInterface::boss_list boss,
           std::uint32_t hp, std::uint32_t players, std::uint32_t cycle, bool explode_on_damage)
: ii::Ship{sim, position, static_cast<Ship::ship_category>(kShipBoss | kShipEnemy)}
, flag_{boss}
, explode_on_damage_{explode_on_damage} {
  set_bounding_width(640);
  set_ignore_damage_colour_index(100);
  auto s = 5000 * (cycle + 1) + 2500 * (boss > ii::SimInterface::kBoss1C) +
      2500 * (boss > ii::SimInterface::kBoss2C);

  score_ += s;
  for (std::uint32_t i = 0; i < players - 1; ++i) {
    score_ += (s * kHpPerExtraPlayer).to_int();
  }
  hp_ = CalculateHP(hp, players, cycle);
  max_hp_ = hp_;
}

std::uint32_t Boss::CalculateHP(std::uint32_t base, std::uint32_t players, std::uint32_t cycle) {
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

void Boss::damage(std::uint32_t damage, bool magic, Player* source) {
  std::uint32_t actual_damage = get_damage(damage, magic);
  if (actual_damage <= 0) {
    return;
  }

  if (damage >= Player::kBombDamage) {
    if (explode_on_damage_) {
      explosion();
      explosion(glm::vec4{1.f}, 16);
      explosion(std::nullopt, 24);
    }

    damaged_ = 25;
  } else if (damaged_ < 1) {
    damaged_ = 1;
  }

  actual_damage *= 60 / (1 + (sim().get_lives() ? sim().player_count() : sim().alive_players()));
  hp_ = hp_ < actual_damage ? 0 : hp_ - actual_damage;

  if (!hp_ && !is_destroyed()) {
    destroy();
    on_destroy();
  } else if (!is_destroyed()) {
    play_sound_random(ii::sound::kEnemyShatter);
  }
}

void Boss::render() const {
  render(true);
}

void Boss::render(bool hp_bar) const {
  if (hp_bar) {
    render_hp_bar();
  }

  if (!damaged_) {
    Ship::render();
    return;
  }
  for (std::size_t i = 0; i < shapes().size(); ++i) {
    shapes()[i]->render(sim(), to_float(shape().centre), shape().rotation().to_float(),
                        i < ignore_damage_colour_ ? glm::vec4{1.f} : glm::vec4{0.f});
  }
  damaged_--;
}

void Boss::render_hp_bar() const {
  if (!show_hp_ && is_on_screen()) {
    show_hp_ = true;
  }

  if (show_hp_) {
    sim().render_hp_bar(static_cast<float>(hp_) / max_hp_);
  }
}

void Boss::on_destroy() {
  set_killed();
  for (const auto& ship : sim().all_ships(kShipEnemy)) {
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

  for (const auto& player : sim().players()) {
    Player* p = (Player*)player;
    if (!p->is_killed() && get_score() > 0) {
      p->add_score(get_score() / sim().alive_players());
    }
  }
}
