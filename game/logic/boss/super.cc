#include "game/logic/boss/super.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kSbBaseHp = 520;
const std::int32_t kSbArcHp = 75;
}  // namespace

SuperBossArc::SuperBossArc(const vec2& position, std::int32_t players, std::int32_t cycle,
                           std::int32_t i, Ship* boss, std::int32_t timer)
: Boss{position, SimState::boss_list(0), kSbArcHp, players, cycle}
, boss_{boss}
, i_{i}
, timer_{timer} {
  add_new_shape<PolyArc>(vec2{}, 140, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 135, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 130, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 125, 32, 2, 0, i * 2 * fixed_c::pi / 16, kShield);
  add_new_shape<PolyArc>(vec2{}, 120, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 115, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 110, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{}, 105, 32, 2, 0, i * 2 * fixed_c::pi / 16, kShield);
}

void SuperBossArc::update() {
  shape().rotate(fixed(6) / 1000);
  for (std::int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour =
        z::colour_cycle(is_hp_low() ? 0xff000099 : 0xff0000ff, (i * 32 + 2 * timer_) % 256);
  }
  ++timer_;
  ++stimer_;
  if (stimer_ == 64) {
    shapes()[0]->category = kDangerous | kVulnerable;
  }
}

void SuperBossArc::render() const {
  if (stimer_ >= 64 || stimer_ % 4 < 2) {
    Boss::render(false);
  }
}

std::int32_t SuperBossArc::get_damage(std::int32_t damage, bool magic) {
  if (damage >= Player::kBombDamage) {
    return damage / 2;
  }
  return damage;
}

void SuperBossArc::on_destroy() {
  vec2 d = vec2::from_polar(i_ * 2 * fixed_c::pi / 16 + shape().rotation(), 120);
  move(d);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);
  play_sound_random(Lib::sound::kExplosion);
  ((SuperBoss*)boss_)->destroyed_[i_] = true;
}

SuperBoss::SuperBoss(std::int32_t players, std::int32_t cycle)
: Boss{{Lib::kWidth / 2, -Lib::kHeight / (2 + fixed_c::half)},
       SimState::BOSS_3A,
       kSbBaseHp,
       players,
       cycle}
, players_{players}
, cycle_{cycle} {
  add_new_shape<Polygon>(vec2{}, 40, 32, 0, 0, kDangerous | kVulnerable);
  add_new_shape<Polygon>(vec2{}, 35, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{}, 30, 32, 0, 0, kShield);
  add_new_shape<Polygon>(vec2{}, 25, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{}, 20, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{}, 15, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{}, 10, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{}, 5, 32, 0, 0, 0);
  for (std::int32_t i = 0; i < 16; ++i) {
    destroyed_.push_back(false);
  }
  set_ignore_damage_colour_index(8);
}

void SuperBoss::update() {
  if (arcs_.empty()) {
    for (std::int32_t i = 0; i < 16; ++i) {
      auto s = std::make_unique<SuperBossArc>(shape().centre, players_, cycle_, i, this);
      arcs_.push_back(s.get());
      spawn(std::move(s));
    }
  } else {
    for (std::int32_t i = 0; i < 16; ++i) {
      if (destroyed_[i]) {
        continue;
      }
      arcs_[i]->shape().centre = shape().centre;
    }
  }
  vec2 move_vec;
  shape().rotate(fixed(6) / 1000);
  colour_t c = z::colour_cycle(0xff0000ff, 2 * ctimer_);
  for (std::int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = z::colour_cycle(0xff0000ff, (i * 32 + 2 * ctimer_) % 256);
  }
  ++ctimer_;
  if (shape().centre.y < Lib::kHeight / 2) {
    move_vec = {0, 1};
  } else if (state_ == state::kArrive) {
    state_ = state::kIdle;
  }

  ++timer_;
  if (state_ == state::kAttack && timer_ == 192) {
    timer_ = 100;
    state_ = state::kIdle;
  }

  static const fixed d5d1000 = fixed(5) / 1000;
  static const fixed pi2d64 = 2 * fixed_c::pi / 64;
  static const fixed pi2d32 = 2 * fixed_c::pi / 32;

  if (state_ == state::kIdle && is_on_screen() && timer_ != 0 && timer_ % 300 == 0) {
    std::int32_t r = z::rand_int(6);
    if (r == 0 || r == 1) {
      snakes_ = 16;
    } else if (r == 2 || r == 3) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = z::rand_fixed() * (2 * fixed_c::pi);
      fixed rf = d5d1000 * (1 + z::rand_int(2));
      for (std::int32_t i = 0; i < 32; ++i) {
        vec2 d = vec2::from_polar(f + i * pi2d32, 1);
        if (r == 2) {
          rf = d5d1000 * (1 + z::rand_int(4));
        }
        spawn_new<Snake>(shape().centre + d * 16, c, d, rf);
        play_sound_random(Lib::sound::kBossAttack);
      }
    } else if (r == 5) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = z::rand_fixed() * (2 * fixed_c::pi);
      for (std::int32_t i = 0; i < 64; ++i) {
        vec2 d = vec2::from_polar(f + i * pi2d64, 1);
        spawn_new<Snake>(shape().centre + d * 16, c, d);
        play_sound_random(Lib::sound::kBossAttack);
      }
    } else {
      state_ = state::kAttack;
      timer_ = 0;
      snakes_ = 32;
    }
  }

  if (state_ == state::kIdle && is_on_screen() && timer_ % 300 == 200 && !is_destroyed()) {
    std::vector<std::int32_t> wide3;
    std::int32_t timer = 0;
    for (std::int32_t i = 0; i < 16; ++i) {
      if (destroyed_[(i + 15) % 16] && destroyed_[i] && destroyed_[(i + 1) % 16]) {
        wide3.push_back(i);
      }
      if (!destroyed_[i]) {
        timer = arcs_[i]->GetTimer();
      }
    }
    if (!wide3.empty()) {
      std::int32_t r = z::rand_int(static_cast<std::int32_t>(wide3.size()));
      auto s =
          std::make_unique<SuperBossArc>(shape().centre, players_, cycle_, wide3[r], this, timer);
      s->shape().set_rotation(shape().rotation() - (fixed(6) / 1000));
      arcs_[wide3[r]] = s.get();
      spawn(std::move(s));
      destroyed_[wide3[r]] = false;
      play_sound(Lib::sound::kEnemySpawn);
    }
  }
  static const fixed pi2d128 = 2 * fixed_c::pi / 128;
  if (state_ == state::kIdle && timer_ % 72 == 0) {
    for (std::int32_t i = 0; i < 128; ++i) {
      vec2 d = vec2::from_polar(i * pi2d128, 1);
      spawn_new<RainbowShot>(shape().centre + d * 42, move_vec + d * 3, this);
      play_sound_random(Lib::sound::kBossFire);
    }
  }

  if (snakes_) {
    --snakes_;
    vec2 d =
        vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), z::rand_int(32) + z::rand_int(16));
    spawn_new<Snake>(d + shape().centre, c);
    play_sound_random(Lib::sound::kEnemySpawn);
  }
  move(move_vec);
}

std::int32_t SuperBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

void SuperBoss::on_destroy() {
  set_killed();
  for (const auto& ship : game().all_ships(kShipEnemy)) {
    if (ship != this) {
      ship->damage(Player::kBombDamage * 100, false, 0);
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
    colour_t c = z::colour_cycle(shapes()[0]->colour, n * 2);
    fireworks_.push_back(std::make_pair(n, std::make_pair(shape().centre + v, c)));
    n += i;
  }
  for (std::int32_t i = 0; i < kPlayers; ++i) {
    lib().rumble(i, 25);
  }
  play_sound(Lib::sound::kExplosion);

  for (const auto& ship : game().players()) {
    Player* p = (Player*)ship;
    if (!p->is_killed()) {
      p->add_score(get_score() / game().alive_players());
    }
  }

  for (std::int32_t i = 0; i < 8; ++i) {
    spawn_new<Powerup>(shape().centre, Powerup::type::kBomb);
  }
}

SnakeTail::SnakeTail(const vec2& position, colour_t colour) : Enemy{position, kShipNone, 1} {
  add_new_shape<Polygon>(vec2{}, 10, 4, colour, 0, kDangerous | kShield | kVulnShield);
  set_bounding_width(22);
  set_score(0);
}

void SnakeTail::update() {
  static const fixed z15 = fixed_c::hundredth * 15;
  shape().rotate(z15);
  --timer_;
  if (!timer_) {
    on_destroy(false);
    destroy();
    explosion();
  }
  if (dtimer_) {
    --dtimer_;
    if (!dtimer_) {
      if (tail_) {
        tail_->dtimer_ = 4;
      }
      on_destroy(false);
      destroy();
      explosion();
      play_sound_random(Lib::sound::kEnemyDestroy);
    }
  }
}

void SnakeTail::on_destroy(bool bomb) {
  if (head_) {
    head_->tail_ = 0;
  }
  if (tail_) {
    tail_->head_ = 0;
  }
  head_ = 0;
  tail_ = 0;
}

Snake::Snake(const vec2& position, colour_t colour, const vec2& dir, fixed rot)
: Enemy{position, kShipNone, 5}, colour_{colour}, shot_rot_{rot} {
  add_new_shape<Polygon>(vec2{}, 14, 3, colour, 0, kVulnerable);
  add_new_shape<Polygon>(vec2{}, 10, 3, 0, 0, kDangerous);
  set_score(0);
  set_bounding_width(32);
  set_enemy_value(5);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  if (dir == vec2{}) {
    std::int32_t r = z::rand_int(4);
    dir_ = r == 0 ? vec2{1, 0} : r == 1 ? vec2{-1, 0} : r == 2 ? vec2{0, 1} : vec2{0, -1};
  } else {
    dir_ = dir.normalised();
    shot_snake_ = true;
  }
  shape().set_rotation(dir_.angle());
}

void Snake::update() {
  if (!(shape().centre.x >= -8 && shape().centre.x <= Lib::kWidth + 8 && shape().centre.y >= -8 &&
        shape().centre.y <= Lib::kHeight + 8)) {
    tail_ = 0;
    destroy();
    return;
  }

  colour_t c = z::colour_cycle(colour_, timer_ % 256);
  shapes()[0]->colour = c;
  ++timer_;
  if (timer_ % (shot_snake_ ? 4 : 8) == 0) {
    auto t = std::make_unique<SnakeTail>(shape().centre, (c & 0xffffff00) | 0x00000099);
    if (tail_ != 0) {
      tail_->head_ = t.get();
      t->tail_ = tail_;
    }
    play_sound_random(Lib::sound::kBossFire, 0, .5f);
    tail_ = t.get();
    spawn(std::move(t));
  }
  if (!shot_snake_ && timer_ % 48 == 0 && !z::rand_int(3)) {
    dir_ = dir_.rotated((z::rand_int(2) ? 1 : -1) * fixed_c::pi / 2);
    shape().set_rotation(dir_.angle());
  }
  move(dir_ * (shot_snake_ ? 4 : 2));
  if (timer_ % 8 == 0) {
    dir_ = dir_.rotated(8 * shot_rot_);
    shape().set_rotation(dir_.angle());
  }
}

void Snake::on_destroy(bool bomb) {
  if (tail_) {
    tail_->dtimer_ = 4;
  }
}

RainbowShot::RainbowShot(const vec2& position, const vec2& velocity, Ship* boss)
: BossShot{position, velocity}, boss_{boss} {}

void RainbowShot::update() {
  BossShot::update();
  static const vec2 center = {Lib::kWidth / 2, Lib::kHeight / 2};

  if ((shape().centre - center).length() > 100 && timer_ % 2 == 0) {
    const auto& list = game().collision_list(shape().centre, kShield);
    SuperBoss* s = (SuperBoss*)boss_;
    for (std::size_t i = 0; i < list.size(); ++i) {
      bool boss = false;
      for (std::size_t j = 0; j < s->arcs_.size(); ++j) {
        if (list[i] == s->arcs_[j]) {
          boss = true;
          break;
        }
      }
      if (!boss) {
        continue;
      }

      explosion(0, 4, true, to_float(shape().centre - dir_));
      destroy();
      return;
    }
  }

  ++timer_;
  static const fixed r = 6 * (8 * fixed_c::hundredth / 10);
  if (timer_ % 8 == 0) {
    dir_ = dir_.rotated(r);
  }
}
