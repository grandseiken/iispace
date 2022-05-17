#include "game/logic/boss.h"
#include "game/logic/player.h"
#include <algorithm>

std::vector<std::pair<std::int32_t, std::pair<vec2, colour_t>>> Boss::_fireworks;
std::vector<vec2> Boss::_warnings;

namespace {
const fixed kHpPerExtraPlayer = fixed(1) / 10;
const fixed kHpPerExtraCycle = 3 * fixed(1) / 10;
}  // namespace

Boss::Boss(const vec2& position, SimState::boss_list boss, std::int32_t hp, std::int32_t players,
           std::int32_t cycle, bool explode_on_damage)
: Ship(position, Ship::ship_category(kShipBoss | kShipEnemy))
, _hp(0)
, _max_hp(0)
, _flag(boss)
, _score(0)
, _ignore_damage_colour(256)
, _damaged(0)
, _show_hp(false)
, _explode_on_damage(explode_on_damage) {
  set_bounding_width(640);
  set_ignore_damage_colour_index(100);
  long s =
      5000 * (cycle + 1) + 2500 * (boss > SimState::BOSS_1C) + 2500 * (boss > SimState::BOSS_2C);

  _score += s;
  for (std::int32_t i = 0; i < players - 1; ++i) {
    _score += (std::int32_t(s) * kHpPerExtraPlayer).to_int();
  }
  _hp = CalculateHP(hp, players, cycle);
  _max_hp = _hp;
}

std::int32_t Boss::CalculateHP(std::int32_t base, std::int32_t players, std::int32_t cycle) {
  std::int32_t hp = base * 30;
  std::int32_t r = hp;
  for (std::int32_t i = 0; i < cycle; ++i) {
    r += (hp * kHpPerExtraCycle).to_int();
  }
  for (std::int32_t i = 0; i < players - 1; ++i) {
    r += (hp * kHpPerExtraPlayer).to_int();
  }
  for (std::int32_t i = 0; i < cycle * (players - 1); ++i) {
    r += (hp * kHpPerExtraCycle + kHpPerExtraPlayer).to_int();
  }
  return r;
}

void Boss::damage(std::int32_t damage, bool magic, Player* source) {
  std::int32_t actual_damage = get_damage(damage, magic);
  if (actual_damage <= 0) {
    return;
  }

  if (damage >= Player::kBombDamage) {
    if (_explode_on_damage) {
      explosion();
      explosion(0xffffffff, 16);
      explosion(0, 24);
    }

    _damaged = 25;
  } else if (_damaged < 1) {
    _damaged = 1;
  }

  actual_damage *=
      60 / (1 + (game().get_lives() ? game().players().size() : game().alive_players()));
  _hp -= actual_damage;

  if (_hp <= 0 && !is_destroyed()) {
    destroy();
    on_destroy();
  } else if (!is_destroyed()) {
    play_sound_random(Lib::sound::kEnemyShatter);
  }
}

void Boss::render() const {
  render(true);
}

void Boss::render(bool hp_bar) const {
  if (hp_bar) {
    render_hp_bar();
  }

  if (!_damaged) {
    Ship::render();
    return;
  }
  for (std::size_t i = 0; i < shapes().size(); i++) {
    shapes()[i]->render(lib(), to_float(shape().centre), shape().rotation().to_float(),
                        std::int32_t(i) < _ignore_damage_colour ? 0xffffffff : 0);
  }
  _damaged--;
}

void Boss::render_hp_bar() const {
  if (!_show_hp && is_on_screen()) {
    _show_hp = true;
  }

  if (_show_hp) {
    game().render_hp_bar(float(_hp) / float(_max_hp));
  }
}

void Boss::on_destroy() {
  set_killed();
  for (const auto& ship : game().all_ships(kShipEnemy)) {
    if (ship != this) {
      ship->damage(Player::kBombDamage, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);
  std::int32_t n = 1;
  for (std::int32_t i = 0; i < 16; ++i) {
    vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi),
                              8 + z::rand_int(64) + z::rand_int(64));
    _fireworks.push_back(
        std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
    n += i;
  }
  for (std::int32_t i = 0; i < kPlayers; i++) {
    lib().rumble(i, 25);
  }
  play_sound(Lib::sound::kExplosion);

  for (const auto& player : game().players()) {
    Player* p = (Player*)player;
    if (!p->is_killed() && get_score() > 0) {
      p->add_score(get_score() / game().alive_players());
    }
  }
}
