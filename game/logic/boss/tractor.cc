#include "game/logic/boss/boss_internal.h"
#include "game/logic/player.h"

namespace {
const std::uint32_t kTbBaseHp = 900;
const std::uint32_t kTbTimer = 100;
const fixed kTbSpeed = 2;
const fixed kTractorBeamSpeed = 2 + 1_fx / 2;

const glm::vec4 c0 = colour_hue360(300, .5f, .6f);
const glm::vec4 c1 = colour_hue360(300, 1.f / 3, .6f);
const glm::vec4 c2 = colour_hue360(300, .4f, .5f);

class TBossShot : public Enemy {
public:
  TBossShot(ii::SimInterface& sim, const vec2& position, fixed angle);
  void update() override;

private:
  vec2 dir_{0};
};

TBossShot::TBossShot(ii::SimInterface& sim, const vec2& position, fixed angle)
: Enemy{sim, position, ii::ship_flag::kNone} {
  add_new_shape<ii::Polygon>(vec2{0}, 8, 6, c0, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
  dir_ = from_polar(angle, 3_fx);
}

void TBossShot::update() {
  if ((shape().centre.x > ii::kSimDimensions.x && dir_.x > 0) ||
      (shape().centre.x < 0 && dir_.x < 0)) {
    dir_.x = -dir_.x;
  }
  if ((shape().centre.y > ii::kSimDimensions.y && dir_.y > 0) ||
      (shape().centre.y < 0 && dir_.y < 0)) {
    dir_.y = -dir_.y;
  }

  move(dir_);
}

void spawn_tboss_shot(ii::SimInterface& sim, const vec2& position, fixed angle) {
  auto h = sim.create_legacy(std::make_unique<TBossShot>(sim, position, angle));
  h.add(ii::legacy_collision(/* bounding width */ 8));
  h.add(ii::Enemy{.threat_value = 1});
  h.add(ii::Health{.hp = 1,
                   .destroy_sound = ii::sound::kEnemyShatter,
                   .on_destroy = &ii::legacy_enemy_on_destroy});
}

class TractorBoss : public Boss {
public:
  TractorBoss(ii::SimInterface& sim);

  void update() override;
  void render() const override;

private:
  ii::CompoundShape* s1_ = nullptr;
  ii::CompoundShape* s2_ = nullptr;
  ii::Polygon* sattack_ = nullptr;
  bool will_attack_ = false;
  bool stopped_ = false;
  bool generating_ = false;
  bool attacking_ = false;
  bool continue_ = false;
  bool gen_dir_ = false;
  std::uint32_t shoot_type_ = 0;
  bool sound_ = true;
  std::uint32_t timer_ = 0;
  std::uint32_t attack_size_ = 0;
  std::size_t attack_shapes_ = 0;

  std::vector<vec2> targets_;
};

TractorBoss::TractorBoss(ii::SimInterface& sim)
: Boss{sim, {ii::kSimDimensions.x * (1 + fixed_c::half), ii::kSimDimensions.y / 2}}
, shoot_type_{sim.random(2)} {
  s1_ = add_new_shape<ii::CompoundShape>(vec2{0, -96}, 0,
                                         ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);

  s1_->add_new_shape<ii::Polygon>(vec2{0}, 12, 6, c1, 0, ii::shape_flag::kNone,
                                  ii::Polygon::T::kPolygram);
  s1_->add_new_shape<ii::Polygon>(vec2{0}, 12, 12, c1, 0);
  s1_->add_new_shape<ii::Polygon>(vec2{0}, 2, 6, c1, 0);
  s1_->add_new_shape<ii::Polygon>(vec2{0}, 36, 12, c0, 0);
  s1_->add_new_shape<ii::Polygon>(vec2{0}, 34, 12, c0, 0);
  s1_->add_new_shape<ii::Polygon>(vec2{0}, 32, 12, c0, 0);
  for (std::uint32_t i = 0; i < 8; ++i) {
    vec2 d = rotate(vec2{24, 0}, i * fixed_c::pi / 4);
    s1_->add_new_shape<ii::Polygon>(d, 12, 6, c0, 0, ii::shape_flag::kNone,
                                    ii::Polygon::T::kPolygram);
  }

  s2_ = add_new_shape<ii::CompoundShape>(vec2{0, 96}, 0,
                                         ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);

  s2_->add_new_shape<ii::Polygon>(vec2{0}, 12, 6, c1, 0, ii::shape_flag::kNone,
                                  ii::Polygon::T::kPolygram);
  s2_->add_new_shape<ii::Polygon>(vec2{0}, 12, 12, c1, 0);
  s2_->add_new_shape<ii::Polygon>(vec2{0}, 2, 6, c1, 0);

  s2_->add_new_shape<ii::Polygon>(vec2{0}, 36, 12, c0, 0);
  s2_->add_new_shape<ii::Polygon>(vec2{0}, 34, 12, c0, 0);
  s2_->add_new_shape<ii::Polygon>(vec2{0}, 32, 12, c0, 0);
  for (std::uint32_t i = 0; i < 8; ++i) {
    vec2 d = rotate(vec2{24, 0}, i * fixed_c::pi / 4);
    s2_->add_new_shape<ii::Polygon>(d, 12, 6, c0, 0, ii::shape_flag::kNone,
                                    ii::Polygon::T::kPolygram);
  }

  sattack_ = add_new_shape<ii::Polygon>(vec2{0}, 0, 16, c2);

  add_new_shape<ii::Line>(vec2{0}, vec2{-2, -96}, vec2{-2, 96}, c0, 0);
  add_new_shape<ii::Line>(vec2{0}, vec2{0, -96}, vec2{0, 96}, c1, 0);
  add_new_shape<ii::Line>(vec2{0}, vec2{2, -96}, vec2{2, 96}, c0, 0);

  add_new_shape<ii::Polygon>(vec2{0, 96}, 30, 12, glm::vec4{0.f}, 0, ii::shape_flag::kShield);
  add_new_shape<ii::Polygon>(vec2{0, -96}, 30, 12, glm::vec4{0.f}, 0, ii::shape_flag::kShield);

  attack_shapes_ = shapes().size();
}

void TractorBoss::update() {
  if (shape().centre.x <= ii::kSimDimensions.x / 2 && will_attack_ && !stopped_ && !continue_) {
    stopped_ = true;
    generating_ = true;
    gen_dir_ = sim().random(2) == 0;
    timer_ = 0;
  }

  if (shape().centre.x < -150) {
    shape().centre.x = ii::kSimDimensions.x + 150;
    will_attack_ = !will_attack_;
    shoot_type_ = sim().random(2);
    if (will_attack_) {
      shoot_type_ = 0;
    }
    continue_ = false;
    sound_ = !will_attack_;
    shape().rotate(sim().random_fixed() * 2 * fixed_c::pi);
  }

  ++timer_;
  bool is_hp_low = handle().get<ii::Health>()->is_hp_low();
  if (!stopped_) {
    move(kTbSpeed * vec2{-1, 0});
    if (!will_attack_ && is_on_screen() && timer_ % (16 - sim().alive_players() * 2) == 0) {
      if (shoot_type_ == 0 || (is_hp_low && shoot_type_ == 1)) {
        auto p = sim().nearest_player_position(shape().centre);

        auto v = s1_->convert_point(shape().centre, shape().rotation(), vec2{0});
        auto d = normalise(p - v);
        ii::spawn_boss_shot(sim(), v, d * 5, c0);
        ii::spawn_boss_shot(sim(), v, d * -5, c0);

        v = s2_->convert_point(shape().centre, shape().rotation(), vec2{0});
        d = normalise(p - v);
        ii::spawn_boss_shot(sim(), v, d * 5, c0);
        ii::spawn_boss_shot(sim(), v, d * -5, c0);

        play_sound_random(ii::sound::kBossFire);
      }
      if (shoot_type_ == 1 || is_hp_low) {
        auto d = sim().nearest_player_direction(shape().centre);
        ii::spawn_boss_shot(sim(), shape().centre, d * 5, c0);
        ii::spawn_boss_shot(sim(), shape().centre, d * -5, c0);
        play_sound_random(ii::sound::kBossFire);
      }
    }
    if ((!will_attack_ || continue_) && is_on_screen()) {
      if (sound_) {
        play_sound(ii::sound::kBossAttack);
        sound_ = false;
      }
      targets_.clear();
      for (const auto& ship : sim().all_ships(ii::ship_flag::kPlayer)) {
        if (((Player*)ship)->is_killed()) {
          continue;
        }
        auto pos = ship->position();
        targets_.push_back(pos);
        auto d = normalise(shape().centre - pos);
        ship->position() += d * kTractorBeamSpeed;
      }
    }
  } else {
    if (generating_) {
      if (timer_ >= kTbTimer * 5) {
        timer_ = 0;
        generating_ = false;
        attacking_ = false;
        attack_size_ = 0;
        play_sound(ii::sound::kBossAttack);
      }

      if (timer_ < kTbTimer * 4 && timer_ % (10 - 2 * sim().alive_players()) == 0) {
        spawn_tboss_shot(sim(), s1_->convert_point(shape().centre, shape().rotation(), vec2{0}),
                         gen_dir_ ? shape().rotation() + fixed_c::pi : shape().rotation());

        spawn_tboss_shot(sim(), s2_->convert_point(shape().centre, shape().rotation(), vec2{0}),
                         shape().rotation() + (gen_dir_ ? 0 : fixed_c::pi));
        play_sound_random(ii::sound::kEnemySpawn);
      }

      if (is_hp_low && timer_ % (20 - sim().alive_players() * 2) == 0) {
        auto d = sim().nearest_player_direction(shape().centre);
        ii::spawn_boss_shot(sim(), shape().centre, d * 5, c0);
        ii::spawn_boss_shot(sim(), shape().centre, d * -5, c0);
        play_sound_random(ii::sound::kBossFire);
      }
    } else {
      if (!attacking_) {
        if (timer_ >= kTbTimer * 4) {
          timer_ = 0;
          attacking_ = true;
        }
        if (timer_ % (kTbTimer / (1 + fixed_c::half)).to_int() == kTbTimer / 8) {
          auto v = s1_->convert_point(shape().centre, shape().rotation(), vec2{0});
          auto d = from_polar(sim().random_fixed() * (2 * fixed_c::pi), 5_fx);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);

          v = s2_->convert_point(shape().centre, shape().rotation(), vec2{0});
          d = from_polar(sim().random_fixed() * (2 * fixed_c::pi), 5_fx);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);
          d = rotate(d, fixed_c::pi / 2);
          ii::spawn_boss_shot(sim(), v, d, c0);
          play_sound_random(ii::sound::kBossFire);
        }
        targets_.clear();
        for (const auto& ship : sim().all_ships(ii::ship_flag::kPlayer | ii::ship_flag::kEnemy)) {
          if (ship == this ||
              (+(ship->type() & ii::ship_flag::kPlayer) && ((Player*)ship)->is_killed())) {
            continue;
          }

          if (+(ship->type() & ii::ship_flag::kEnemy)) {
            play_sound_random(ii::sound::kBossAttack, 0, 0.3f);
          }
          auto pos = ship->position();
          targets_.push_back(pos);
          fixed speed = 0;
          if (+(ship->type() & ii::ship_flag::kPlayer)) {
            speed = kTractorBeamSpeed;
          }
          if (+(ship->type() & ii::ship_flag::kEnemy)) {
            speed = 4 + fixed_c::half;
          }
          auto d = normalise(shape().centre - pos);
          ship->position() += d * speed;

          if (+(ship->type() & ii::ship_flag::kEnemy) && !(ship->type() & ii::ship_flag::kWall) &&
              length(ship->position() - shape().centre) <= 40) {
            ship->destroy();
            ++attack_size_;
            sattack_->radius = attack_size_ / (1 + fixed_c::half);
            add_new_shape<ii::Polygon>(vec2{0}, 8, 6, c0, 0);
          }
        }
      } else {
        timer_ = 0;
        stopped_ = false;
        continue_ = true;
        for (std::uint32_t i = 0; i < attack_size_; ++i) {
          vec2 d = from_polar(i * (2 * fixed_c::pi) / attack_size_, 5_fx);
          ii::spawn_boss_shot(sim(), shape().centre, d, c0);
        }
        play_sound(ii::sound::kBossFire);
        play_sound_random(ii::sound::kExplosion);
        attack_size_ = 0;
        sattack_->radius = 0;
        while (attack_shapes_ < shapes().size()) {
          destroy_shape(attack_shapes_);
        }
      }
    }
  }

  for (std::size_t i = attack_shapes_; i < shapes().size(); ++i) {
    vec2 v = from_polar(
        sim().random_fixed() * (2 * fixed_c::pi),
        2 * (sim().random_fixed() - fixed_c::half) * fixed{attack_size_} / (1 + fixed_c::half));
    shapes()[i]->centre = v;
  }

  fixed r = 0;
  if (!will_attack_ || (stopped_ && !generating_ && !attacking_)) {
    r = fixed_c::hundredth;
  } else if (stopped_ && attacking_ && !generating_) {
    r = 0;
  } else {
    r = fixed_c::hundredth / 2;
  }
  shape().rotate(r);

  s1_->rotate(fixed_c::tenth / 2);
  s2_->rotate(-fixed_c::tenth / 2);
  for (std::size_t i = 0; i < 8; ++i) {
    s1_->shapes()[4 + i]->rotate(-fixed_c::tenth);
    s2_->shapes()[4 + i]->rotate(fixed_c::tenth);
  }
}

void TractorBoss::render() const {
  Boss::render();
  if ((stopped_ && !generating_ && !attacking_) ||
      (!stopped_ && (continue_ || !will_attack_) && is_on_screen())) {
    for (std::size_t i = 0; i < targets_.size(); ++i) {
      if (((timer_ + i * 4) / 4) % 2) {
        sim().render_line(to_float(shape().centre), to_float(targets_[i]), c0);
      }
    }
  }
}

}  // namespace

namespace ii {
void spawn_tractor_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<TractorBoss>(sim));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(kTbBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = ii::sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss2A});
}
}  // namespace ii