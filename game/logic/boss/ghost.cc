#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"

namespace {
const std::uint32_t kGbBaseHp = 700;
const std::uint32_t kGbTimer = 250;
const std::uint32_t kGbAttackTime = 100;
const fixed kGbWallSpeed = 3 + 1_fx / 2;

const glm::vec4 c0 = colour_hue360(280, .7f);
const glm::vec4 c1 = colour_hue360(280, .5f, .6f);
const glm::vec4 c2 = colour_hue360(270, .2f);

class GhostWall : public Enemy {
public:
  GhostWall(ii::SimInterface& sim, bool swap, bool no_gap, bool ignored);
  GhostWall(ii::SimInterface& sim, bool swap, bool swap_gap);
  void update() override;

private:
  vec2 dir_;
};

GhostWall::GhostWall(ii::SimInterface& sim, bool swap, bool no_gap, bool ignored)
: Enemy{sim,
        {ii::kSimDimensions.x / 2, swap ? -10 : 10 + ii::kSimDimensions.y},
        ii::ship_flag::kNone,
        0}
, dir_{0, swap ? 1 : -1} {
  if (no_gap) {
    add_new_shape<ii::Box>(vec2{0}, fixed{ii::kSimDimensions.x}, 10, c0, 0,
                           ii::shape_flag::kDangerous | ii::shape_flag::kShield);
    add_new_shape<ii::Box>(vec2{0}, fixed{ii::kSimDimensions.x}, 7, c0, 0);
    add_new_shape<ii::Box>(vec2{0}, fixed{ii::kSimDimensions.x}, 4, c0, 0);
  } else {
    add_new_shape<ii::Box>(vec2{0}, ii::kSimDimensions.y / 2, 10, c0, 0,
                           ii::shape_flag::kDangerous | ii::shape_flag::kShield);
    add_new_shape<ii::Box>(vec2{0}, ii::kSimDimensions.y / 2, 7, c0, 0,
                           ii::shape_flag::kDangerous | ii::shape_flag::kShield);
    add_new_shape<ii::Box>(vec2{0}, ii::kSimDimensions.y / 2, 4, c0, 0,
                           ii::shape_flag::kDangerous | ii::shape_flag::kShield);
  }
}

GhostWall::GhostWall(ii::SimInterface& sim, bool swap, bool swap_gap)
: Enemy{sim,
        {swap ? -10 : 10 + ii::kSimDimensions.x, ii::kSimDimensions.y / 2},
        ii::ship_flag::kNone,
        0}
, dir_{swap ? 1 : -1, 0} {
  fixed off = swap_gap ? -100 : 100;

  add_new_shape<ii::Box>(vec2{0, off - 20 - ii::kSimDimensions.y / 2}, 10, ii::kSimDimensions.y / 2,
                         c0, 0, ii::shape_flag::kDangerous | ii::shape_flag::kShield);
  add_new_shape<ii::Box>(vec2{0, off + 20 + ii::kSimDimensions.y / 2}, 10, ii::kSimDimensions.y / 2,
                         c0, 0, ii::shape_flag::kDangerous | ii::shape_flag::kShield);

  add_new_shape<ii::Box>(vec2{0, off - 20 - ii::kSimDimensions.y / 2}, 7, ii::kSimDimensions.y / 2,
                         c0, 0);
  add_new_shape<ii::Box>(vec2{0, off + 20 + ii::kSimDimensions.y / 2}, 7, ii::kSimDimensions.y / 2,
                         c0, 0);

  add_new_shape<ii::Box>(vec2{0, off - 20 - ii::kSimDimensions.y / 2}, 4, ii::kSimDimensions.y / 2,
                         c0, 0);
  add_new_shape<ii::Box>(vec2{0, off + 20 + ii::kSimDimensions.y / 2}, 4, ii::kSimDimensions.y / 2,
                         c0, 0);
}

void GhostWall::update() {
  if ((dir_.x > 0 && shape().centre.x < 32) || (dir_.y > 0 && shape().centre.y < 16) ||
      (dir_.x < 0 && shape().centre.x >= ii::kSimDimensions.x - 32) ||
      (dir_.y < 0 && shape().centre.y >= ii::kSimDimensions.y - 16)) {
    move(dir_ * kGbWallSpeed / 2);
  } else {
    move(dir_ * kGbWallSpeed);
  }

  if ((dir_.x > 0 && shape().centre.x > ii::kSimDimensions.x + 10) ||
      (dir_.y > 0 && shape().centre.y > ii::kSimDimensions.y + 10) ||
      (dir_.x < 0 && shape().centre.x < -10) || (dir_.y < 0 && shape().centre.y < -10)) {
    destroy();
  }
}

class GhostMine : public Enemy {
public:
  GhostMine(ii::SimInterface& sim, const vec2& position, Boss* ghost);
  void update() override;
  void render() const override;

private:
  std::uint32_t timer_ = 80;
  Boss* ghost_ = nullptr;
};

GhostMine::GhostMine(ii::SimInterface& sim, const vec2& position, Boss* ghost)
: Enemy{sim, position, ii::ship_flag::kNone, 0}, ghost_{ghost} {
  add_new_shape<ii::Polygon>(vec2{0}, 24, 8, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 20, 8, c1, 0);
}

void GhostMine::update() {
  if (timer_ == 80) {
    explosion();
    shape().set_rotation(sim().random_fixed() * 2 * fixed_c::pi);
  }
  if (timer_) {
    timer_--;
    if (!timer_) {
      shapes()[0]->category =
          ii::shape_flag::kDangerous | ii::shape_flag::kShield | ii::shape_flag::kVulnShield;
    }
  }
  for (const auto& ship : sim().collision_list(shape().centre, ii::shape_flag::kDangerous)) {
    if (ship == ghost_) {
      if (sim().random(6) == 0 || (ghost_->is_hp_low() && sim().random(5) == 0)) {
        ii::spawn_big_follow(sim(), shape().centre, /* score */ false);
      } else {
        ii::spawn_follow(sim(), shape().centre, /* score */ false);
      }
      damage(1, false, 0);
      break;
    }
  }
}

void GhostMine::render() const {
  if (!((timer_ / 4) % 2)) {
    Enemy::render();
  }
}

void spawn_ghost_wall(ii::SimInterface& sim, bool swap, bool no_gap, bool ignored) {
  auto h = sim.create_legacy(std::make_unique<GhostWall>(sim, swap, no_gap, ignored));
  h.add(ii::legacy_collision(/* bounding width */ ii::kSimDimensions.x, h));
  h.add(ii::Enemy{.threat_value = 1});
}

void spawn_ghost_wall(ii::SimInterface& sim, bool swap, bool swap_gap) {
  auto h = sim.create_legacy(std::make_unique<GhostWall>(sim, swap, swap_gap));
  h.add(ii::legacy_collision(/* bounding width */ ii::kSimDimensions.y, h));
  h.add(ii::Enemy{.threat_value = 1});
}

void spawn_ghost_mine(ii::SimInterface& sim, const vec2& position, Boss* ghost) {
  auto h = sim.create_legacy(std::make_unique<GhostMine>(sim, position, ghost));
  h.add(ii::legacy_collision(/* bounding width */ 24, h));
  h.add(ii::Enemy{.threat_value = 1});
}

class GhostBoss : public Boss {
public:
  GhostBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle);

  void update() override;
  void render() const override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;

private:
  bool visible_ = false;
  std::uint32_t vtime_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t attack_time_ = 0;
  std::uint32_t attack_ = 0;
  bool _rdir = false;
  std::uint32_t start_time_ = 120;
  std::uint32_t danger_circle_ = 0;
  std::uint32_t danger_offset1_ = 0;
  std::uint32_t danger_offset2_ = 0;
  std::uint32_t danger_offset3_ = 0;
  std::uint32_t danger_offset4_ = 0;
  bool shot_type_ = false;
};

GhostBoss::GhostBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle)
: Boss{sim,
       {ii::kSimDimensions.x / 2, ii::kSimDimensions.y / 2},
       ii::SimInterface::kBoss2B,
       kGbBaseHp,
       players,
       cycle}
, attack_time_{kGbAttackTime} {
  add_new_shape<ii::Polygon>(vec2{0}, 32, 8, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 48, 8, c0, 0);

  for (std::uint32_t i = 0; i < 8; ++i) {
    vec2 c = from_polar(i * fixed_c::pi / 4, 48_fx);

    auto* s = add_new_shape<ii::CompoundShape>(c, 0);
    for (std::uint32_t j = 0; j < 8; ++j) {
      vec2 d = from_polar(j * fixed_c::pi / 4, 1_fx);
      s->add_new_shape<ii::Line>(vec2{0}, d * 10, d * 10 * 2, c1, 0);
    }
  }

  add_new_shape<ii::Polygon>(vec2{0}, 24, 8, c0, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolygram);

  for (std::uint32_t n = 0; n < 5; ++n) {
    auto* s = add_new_shape<ii::CompoundShape>(vec2{0}, 0);
    for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
      vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
      s->add_new_shape<ii::Polygon>(d, 16, 8, n ? c2 : c1, 0, ii::shape_flag::kNone,
                                    ii::Polygon::T::kPolystar);
      s->add_new_shape<ii::Polygon>(d, 12, 8, n ? c2 : c1);
    }
  }

  for (std::uint32_t n = 0; n < 5; ++n) {
    add_new_shape<ii::CompoundShape>(vec2{0}, 0, ii::shape_flag::kDangerous);
  }

  set_ignore_damage_colour_index(12);
}

void GhostBoss::update() {
  for (std::uint32_t n = 1; n < 5; ++n) {
    ii::CompoundShape* s = (ii::CompoundShape*)shapes()[11 + n].get();
    ii::CompoundShape* c = (ii::CompoundShape*)shapes()[16 + n].get();
    c->clear_shapes();

    for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
      ii::Shape& s1 = *s->shapes()[2 * i];
      ii::Shape& s2 = *s->shapes()[1 + 2 * i];
      auto t = n == 1 ? (i + danger_offset1_) % 11
          : n == 2    ? (i + danger_offset2_) % 7
          : n == 3    ? (i + danger_offset3_) % 17
                      : (i + danger_offset4_) % 10;

      bool b = n == 1 ? (t % 11 == 0 || t % 11 == 5)
          : n == 2    ? (t % 7 == 0)
          : n == 3    ? (t % 17 == 0 || t % 17 == 8)
                      : (t % 10 == 0);

      if (((n == 4 && attack_ != 2) || (n + (n == 3)) & danger_circle_) && visible_ && b) {
        s1.colour = c0;
        s2.colour = c0;
        if (vtime_ == 0) {
          vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
          c->add_new_shape<ii::Polygon>(d, 9, 8, glm::vec4{0.f});
          warnings_.push_back(c->convert_point(shape().centre, shape().rotation(), d));
        }
      } else {
        s1.colour = c2;
        s2.colour = c2;
      }
    }
  }
  if (!(attack_ == 2 && attack_time_ < kGbAttackTime * 4 && attack_time_)) {
    shape().rotate(-fixed_c::hundredth * 4);
  }

  for (std::size_t i = 0; i < 8; ++i) {
    shapes()[i + 2]->rotate(fixed_c::hundredth * 4);
  }
  shapes()[10]->rotate(fixed_c::hundredth * 6);
  for (std::uint32_t n = 0; n < 5; ++n) {
    ii::CompoundShape* s = (ii::CompoundShape*)shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 5 - (3_fx / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }

    s = (ii::CompoundShape*)shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 4 - (3_fx / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }
  }

  if (start_time_) {
    start_time_--;
    return;
  }

  if (!attack_time_) {
    ++timer_;
    if (timer_ == 9 * kGbTimer / 10 ||
        (timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
         ((timer_ % 16 == 0 && attack_ == 2) || (timer_ % 32 == 0 && shot_type_)))) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        auto d = from_polar(i * fixed_c::pi / 4 + shape().rotation(), 5_fx);
        ii::spawn_boss_shot(sim(), shape().centre, d, c0);
      }
      play_sound_random(ii::sound::kBossFire);
    }
    if (timer_ != 9 * kGbTimer / 10 && timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
        timer_ % 16 == 0 && (!shot_type_ || attack_ == 2)) {
      auto d = sim().nearest_player_direction(shape().centre);
      if (d != vec2{}) {
        ii::spawn_boss_shot(sim(), shape().centre, d * 5, c0);
        ii::spawn_boss_shot(sim(), shape().centre, d * -5, c0);
        play_sound_random(ii::sound::kBossFire);
      }
    }
  } else {
    attack_time_--;

    if (attack_ == 2) {
      if (attack_time_ >= kGbAttackTime * 4 && attack_time_ % 8 == 0) {
        vec2 pos(sim().random(ii::kSimDimensions.x + 1), sim().random(ii::kSimDimensions.y + 1));
        spawn_ghost_mine(sim(), pos, this);
        play_sound_random(ii::sound::kEnemySpawn);
        danger_circle_ = 0;
      } else if (attack_time_ == kGbAttackTime * 4 - 1) {
        visible_ = true;
        vtime_ = 60;
        add_new_shape<ii::Box>(vec2{ii::kSimDimensions.x / 2 + 32, 0}, ii::kSimDimensions.x / 2, 10,
                               c0, 0);
        add_new_shape<ii::Box>(vec2{ii::kSimDimensions.x / 2 + 32, 0}, ii::kSimDimensions.x / 2, 7,
                               c0, 0);
        add_new_shape<ii::Box>(vec2{ii::kSimDimensions.x / 2 + 32, 0}, ii::kSimDimensions.x / 2, 4,
                               c0, 0);

        add_new_shape<ii::Box>(vec2{-ii::kSimDimensions.x / 2 - 32, 0}, ii::kSimDimensions.x / 2,
                               10, c0, 0);
        add_new_shape<ii::Box>(vec2{-ii::kSimDimensions.x / 2 - 32, 0}, ii::kSimDimensions.x / 2, 7,
                               c0, 0);
        add_new_shape<ii::Box>(vec2{-ii::kSimDimensions.x / 2 - 32, 0}, ii::kSimDimensions.x / 2, 4,
                               c0, 0);
        play_sound(ii::sound::kBossFire);
      } else if (attack_time_ < kGbAttackTime * 3) {
        if (attack_time_ == kGbAttackTime * 3 - 1) {
          play_sound(ii::sound::kBossAttack);
        }
        shape().rotate((_rdir ? 1 : -1) * 2 * fixed_c::pi / (kGbAttackTime * 6));
        if (!attack_time_) {
          for (std::uint32_t i = 0; i < 6; ++i) {
            destroy_shape(21);
          }
        }
        danger_offset1_ = sim().random(11);
        danger_offset2_ = sim().random(7);
        danger_offset3_ = sim().random(17);
        danger_offset3_ = sim().random(10);
      }
    }

    if (attack_ == 0 && attack_time_ == kGbAttackTime * 2) {
      bool r = sim().random(2) != 0;
      spawn_ghost_wall(sim(), r, true);
      spawn_ghost_wall(sim(), !r, false);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 2) {
      _rdir = sim().random(2) != 0;
      spawn_ghost_wall(sim(), !_rdir, false, 0);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 1) {
      spawn_ghost_wall(sim(), _rdir, true, 0);
    }

    if ((attack_ == 0 || attack_ == 1) && is_hp_low() && attack_time_ == kGbAttackTime * 1) {
      auto r = sim().random(4);
      vec2 v = r == 0 ? vec2{-ii::kSimDimensions.x / 4, ii::kSimDimensions.y / 2}
          : r == 1 ? vec2{ii::kSimDimensions.x + ii::kSimDimensions.x / 4, ii::kSimDimensions.y / 2}
          : r == 2
          ? vec2{ii::kSimDimensions.x / 2, -ii::kSimDimensions.y / 4}
          : vec2{ii::kSimDimensions.x / 2, ii::kSimDimensions.y + ii::kSimDimensions.y / 4};
      ii::spawn_big_follow(sim(), v, false);
    }
    if (!attack_time_ && !visible_) {
      visible_ = true;
      vtime_ = 60;
    }
    if (attack_time_ == 0 && attack_ != 2) {
      auto r = sim().random(3);
      danger_circle_ |= 1 + (r == 2 ? 3 : r);
      if (sim().random(2) || is_hp_low()) {
        r = sim().random(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
      if (sim().random(2) || is_hp_low()) {
        r = sim().random(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
    } else {
      danger_circle_ = 0;
    }
  }

  if (timer_ >= kGbTimer) {
    timer_ = 0;
    visible_ = false;
    vtime_ = 60;
    attack_ = sim().random(3);
    shot_type_ = sim().random(2) == 0;
    shapes()[0]->category = ii::shape_flag::kNone;
    shapes()[1]->category = ii::shape_flag::kNone;
    shapes()[11]->category = ii::shape_flag::kNone;
    if (shapes().size() >= 22) {
      shapes()[21]->category = ii::shape_flag::kNone;
      shapes()[24]->category = ii::shape_flag::kNone;
    }
    play_sound(ii::sound::kBossAttack);

    if (attack_ == 0) {
      attack_time_ = kGbAttackTime * 2 + 50;
    }
    if (attack_ == 1) {
      attack_time_ = kGbAttackTime * 3;
      for (std::uint32_t i = 0; i < sim().player_count(); ++i) {
        ii::spawn_powerup(sim(), shape().centre, ii::powerup_type::kShield);
      }
    }
    if (attack_ == 2) {
      attack_time_ = kGbAttackTime * 6;
      _rdir = sim().random(2) != 0;
    }
  }

  if (vtime_) {
    vtime_--;
    if (!vtime_ && visible_) {
      shapes()[0]->category = ii::shape_flag::kShield;
      shapes()[1]->category = ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable;
      shapes()[11]->category = ii::shape_flag::kDangerous;
      if (shapes().size() >= 22) {
        shapes()[21]->category = ii::shape_flag::kDangerous;
        shapes()[24]->category = ii::shape_flag::kDangerous;
      }
    }
  }
}

void GhostBoss::render() const {
  if ((start_time_ / 4) % 2 == 1) {
    render_with_colour({0.f, 0.f, 0.f, 1.f / 255});
    return;
  }
  if ((visible_ && ((vtime_ / 4) % 2 == 0)) || (!visible_ && ((vtime_ / 4) % 2 == 1))) {
    Boss::render();
    return;
  }

  render_with_colour(c2);
  if (!start_time_) {
    render_hp_bar();
  }
}

std::uint32_t GhostBoss::get_damage(std::uint32_t damage, bool magic) {
  return damage;
}

}  // namespace

namespace ii {
void spawn_ghost_boss(SimInterface& sim, std::uint32_t players, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<GhostBoss>(sim, players, cycle));
  h.add(legacy_collision(/* bounding width */ 640, h));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward = calculate_boss_score(SimInterface::kBoss2B, players, cycle)});
}
}  // namespace ii