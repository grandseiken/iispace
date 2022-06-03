#include "game/logic/boss/super.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kSbBaseHp = 520;
const std::int32_t kSbArcHp = 75;
}  // namespace

SuperBossArc::SuperBossArc(ii::SimInterface& sim, const vec2& position, std::int32_t players,
                           std::int32_t cycle, std::int32_t i, Ship* boss, std::int32_t timer)
: Boss{sim, position, static_cast<ii::SimInterface::boss_list>(0), kSbArcHp, players, cycle}
, boss_{boss}
, i_{i}
, timer_{timer} {
  add_new_shape<PolyArc>(vec2{0}, 140, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 135, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 130, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 125, 32, 2, 0, i * 2 * fixed_c::pi / 16, kShield);
  add_new_shape<PolyArc>(vec2{0}, 120, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 115, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 110, 32, 2, 0, i * 2 * fixed_c::pi / 16, 0);
  add_new_shape<PolyArc>(vec2{0}, 105, 32, 2, 0, i * 2 * fixed_c::pi / 16, kShield);
}

void SuperBossArc::update() {
  shape().rotate(6_fx / 1000);
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
  vec2 d = from_polar(i_ * 2 * fixed_c::pi / 16 + shape().rotation(), 120_fx);
  move(d);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);
  play_sound_random(ii::sound::kExplosion);
  ((SuperBoss*)boss_)->destroyed_[i_] = true;
}

SuperBoss::SuperBoss(ii::SimInterface& sim, std::int32_t players, std::int32_t cycle)
: Boss{sim,
       {ii::kSimWidth / 2, -ii::kSimHeight / (2 + fixed_c::half)},
       ii::SimInterface::kBoss3A,
       kSbBaseHp,
       players,
       cycle}
, players_{players}
, cycle_{cycle} {
  add_new_shape<Polygon>(vec2{0}, 40, 32, 0, 0, kDangerous | kVulnerable);
  add_new_shape<Polygon>(vec2{0}, 35, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 30, 32, 0, 0, kShield);
  add_new_shape<Polygon>(vec2{0}, 25, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 20, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 15, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 10, 32, 0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 5, 32, 0, 0, 0);
  for (std::int32_t i = 0; i < 16; ++i) {
    destroyed_.push_back(false);
  }
  set_ignore_damage_colour_index(8);
}

void SuperBoss::update() {
  if (arcs_.empty()) {
    for (std::int32_t i = 0; i < 16; ++i) {
      arcs_.push_back(spawn_new<SuperBossArc>(shape().centre, players_, cycle_, i, this));
    }
  } else {
    for (std::int32_t i = 0; i < 16; ++i) {
      if (destroyed_[i]) {
        continue;
      }
      arcs_[i]->shape().centre = shape().centre;
    }
  }
  vec2 move_vec{0};
  shape().rotate(6_fx / 1000);
  colour_t c = z::colour_cycle(0xff0000ff, 2 * ctimer_);
  for (std::int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = z::colour_cycle(0xff0000ff, (i * 32 + 2 * ctimer_) % 256);
  }
  ++ctimer_;
  if (shape().centre.y < ii::kSimHeight / 2) {
    move_vec = {0, 1};
  } else if (state_ == state::kArrive) {
    state_ = state::kIdle;
  }

  ++timer_;
  if (state_ == state::kAttack && timer_ == 192) {
    timer_ = 100;
    state_ = state::kIdle;
  }

  static const fixed d5d1000 = 5_fx / 1000;
  static const fixed pi2d64 = 2 * fixed_c::pi / 64;
  static const fixed pi2d32 = 2 * fixed_c::pi / 32;

  if (state_ == state::kIdle && is_on_screen() && timer_ != 0 && timer_ % 300 == 0) {
    std::int32_t r = sim().random(6);
    if (r == 0 || r == 1) {
      snakes_ = 16;
    } else if (r == 2 || r == 3) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = sim().random_fixed() * (2 * fixed_c::pi);
      fixed rf = d5d1000 * (1 + sim().random(2));
      for (std::int32_t i = 0; i < 32; ++i) {
        vec2 d = from_polar(f + i * pi2d32, 1_fx);
        if (r == 2) {
          rf = d5d1000 * (1 + sim().random(4));
        }
        spawn_new<Snake>(shape().centre + d * 16, c, d, rf);
        play_sound_random(ii::sound::kBossAttack);
      }
    } else if (r == 5) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = sim().random_fixed() * (2 * fixed_c::pi);
      for (std::int32_t i = 0; i < 64; ++i) {
        vec2 d = from_polar(f + i * pi2d64, 1_fx);
        spawn_new<Snake>(shape().centre + d * 16, c, d);
        play_sound_random(ii::sound::kBossAttack);
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
      std::int32_t r = sim().random(static_cast<std::int32_t>(wide3.size()));
      auto* s = spawn_new<SuperBossArc>(shape().centre, players_, cycle_, wide3[r], this, timer);
      s->shape().set_rotation(shape().rotation() - (6_fx / 1000));
      arcs_[wide3[r]] = s;
      destroyed_[wide3[r]] = false;
      play_sound(ii::sound::kEnemySpawn);
    }
  }
  static const fixed pi2d128 = 2 * fixed_c::pi / 128;
  if (state_ == state::kIdle && timer_ % 72 == 0) {
    for (std::int32_t i = 0; i < 128; ++i) {
      vec2 d = from_polar(i * pi2d128, 1_fx);
      spawn_new<RainbowShot>(shape().centre + d * 42, move_vec + d * 3, this);
      play_sound_random(ii::sound::kBossFire);
    }
  }

  if (snakes_) {
    --snakes_;
    vec2 d = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{sim().random(32) + sim().random(16)});
    spawn_new<Snake>(d + shape().centre, c);
    play_sound_random(ii::sound::kEnemySpawn);
  }
  move(move_vec);
}

std::int32_t SuperBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

void SuperBoss::on_destroy() {
  set_killed();
  for (const auto& ship : sim().all_ships(kShipEnemy)) {
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
    vec2 v = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{8 + sim().random(64) + sim().random(64)});
    colour_t c = z::colour_cycle(shapes()[0]->colour, n * 2);
    fireworks_.push_back(std::make_pair(n, std::make_pair(shape().centre + v, c)));
    n += i;
  }
  sim().rumble_all(25);
  play_sound(ii::sound::kExplosion);

  for (const auto& ship : sim().players()) {
    Player* p = (Player*)ship;
    if (!p->is_killed()) {
      p->add_score(get_score() / sim().alive_players());
    }
  }

  for (std::int32_t i = 0; i < 8; ++i) {
    spawn_new<Powerup>(shape().centre, Powerup::type::kBomb);
  }
}

SnakeTail::SnakeTail(ii::SimInterface& sim, const vec2& position, colour_t colour)
: Enemy{sim, position, kShipNone, 1} {
  add_new_shape<Polygon>(vec2{0}, 10, 4, colour, 0, kDangerous | kShield | kVulnShield);
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
      play_sound_random(ii::sound::kEnemyDestroy);
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

Snake::Snake(ii::SimInterface& sim, const vec2& position, colour_t colour, const vec2& dir,
             fixed rot)
: Enemy{sim, position, kShipNone, 5}, colour_{colour}, shot_rot_{rot} {
  add_new_shape<Polygon>(vec2{0}, 14, 3, colour, 0, kVulnerable);
  add_new_shape<Polygon>(vec2{0}, 10, 3, 0, 0, kDangerous);
  set_score(0);
  set_bounding_width(32);
  set_enemy_value(5);
  set_destroy_sound(ii::sound::kPlayerDestroy);
  if (dir == vec2{0}) {
    std::int32_t r = sim.random(4);
    dir_ = r == 0 ? vec2{1, 0} : r == 1 ? vec2{-1, 0} : r == 2 ? vec2{0, 1} : vec2{0, -1};
  } else {
    dir_ = normalise(dir);
    shot_snake_ = true;
  }
  shape().set_rotation(angle(dir_));
}

void Snake::update() {
  if (!(shape().centre.x >= -8 && shape().centre.x <= ii::kSimWidth + 8 && shape().centre.y >= -8 &&
        shape().centre.y <= ii::kSimHeight + 8)) {
    tail_ = 0;
    destroy();
    return;
  }

  colour_t c = z::colour_cycle(colour_, timer_ % 256);
  shapes()[0]->colour = c;
  ++timer_;
  if (timer_ % (shot_snake_ ? 4 : 8) == 0) {
    auto* t = spawn_new<SnakeTail>(shape().centre, (c & 0xffffff00) | 0x00000099);
    if (tail_ != 0) {
      tail_->head_ = t;
      t->tail_ = tail_;
    }
    play_sound_random(ii::sound::kBossFire, 0, .5f);
    tail_ = t;
  }
  if (!shot_snake_ && timer_ % 48 == 0 && !sim().random(3)) {
    dir_ = rotate(dir_, (sim().random(2) ? 1 : -1) * fixed_c::pi / 2);
    shape().set_rotation(angle(dir_));
  }
  move(dir_ * (shot_snake_ ? 4 : 2));
  if (timer_ % 8 == 0) {
    dir_ = rotate(dir_, 8 * shot_rot_);
    shape().set_rotation(angle(dir_));
  }
}

void Snake::on_destroy(bool bomb) {
  if (tail_) {
    tail_->dtimer_ = 4;
  }
}

RainbowShot::RainbowShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                         Ship* boss)
: BossShot{sim, position, velocity}, boss_{boss} {}

void RainbowShot::update() {
  BossShot::update();
  static const vec2 center = {ii::kSimWidth / 2, ii::kSimHeight / 2};

  if (length(shape().centre - center) > 100 && timer_ % 2 == 0) {
    const auto& list = sim().collision_list(shape().centre, kShield);
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
    dir_ = rotate(dir_, r);
  }
}
