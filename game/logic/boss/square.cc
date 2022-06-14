#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"
#include <glm/gtc/constants.hpp>

namespace {
const std::uint32_t kBsbBaseHp = 400;
const std::uint32_t kBsbTimer = 100;
const std::uint32_t kBsbSTimer = 80;
const std::uint32_t kBsbAttackTime = 90;

const fixed kBsbSpeed = 2 + 1_fx / 2;
const fixed kBsbAttackRadius = 120;

const glm::vec4 c0 = colour_hue360(270, .6f);
const glm::vec4 c1 = colour_hue360(270, .4f);
const glm::vec4 c2 = colour_hue360(260, .3f);

class BigSquareBoss : public Boss {
public:
  BigSquareBoss(ii::SimInterface& sim);

  void update() override;
  void render() const override;

private:
  vec2 dir_;
  bool reverse_ = false;
  std::uint32_t timer_ = 0;
  std::uint32_t spawn_timer_ = 0;
  std::uint32_t special_timer_ = 0;
  bool special_attack_ = false;
  bool special_attack_rotate_ = false;
  Player* attack_player_ = nullptr;
};

BigSquareBoss::BigSquareBoss(ii::SimInterface& sim)
: Boss{sim, {ii::kSimDimensions.x * fixed_c::hundredth * 75, ii::kSimDimensions.y * 2}}
, dir_{0, -1}
, timer_{kBsbTimer * 6} {
  add_new_shape<ii::Polygon>(vec2{0}, 160, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 140, 4, c0, 0, ii::shape_flag::kDangerous);
  add_new_shape<ii::Polygon>(vec2{0}, 120, 4, c0, 0, ii::shape_flag::kDangerous);
  add_new_shape<ii::Polygon>(vec2{0}, 100, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 80, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 60, 4, c0, 0, ii::shape_flag::kVulnerable);

  add_new_shape<ii::Polygon>(vec2{0}, 155, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 135, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 115, 4, c0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 95, 4, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 75, 4, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 55, 4, c2, 0, ii::shape_flag::kShield);
}

void BigSquareBoss::update() {
  const vec2& pos = shape().centre;
  if (pos.y < ii::kSimDimensions.y * fixed_c::hundredth * 25 && dir_.y == -1) {
    dir_ = {reverse_ ? 1 : -1, 0};
  }
  if (pos.x < ii::kSimDimensions.x * fixed_c::hundredth * 25 && dir_.x == -1) {
    dir_ = {0, reverse_ ? -1 : 1};
  }
  if (pos.y > ii::kSimDimensions.y * fixed_c::hundredth * 75 && dir_.y == 1) {
    dir_ = {reverse_ ? -1 : 1, 0};
  }
  if (pos.x > ii::kSimDimensions.x * fixed_c::hundredth * 75 && dir_.x == 1) {
    dir_ = {0, reverse_ ? 1 : -1};
  }

  if (special_attack_) {
    special_timer_--;
    if (attack_player_->is_killed()) {
      special_timer_ = 0;
      attack_player_ = 0;
    } else if (!special_timer_) {
      vec2 d(kBsbAttackRadius, 0);
      if (special_attack_rotate_) {
        d = rotate(d, fixed_c::pi / 2);
      }
      for (std::uint32_t i = 0; i < 6; ++i) {
        ii::spawn_follow(sim(), attack_player_->shape().centre + d, /* score */ false,
                         fixed_c::pi / 4);
        d = rotate(d, 2 * fixed_c::pi / 6);
      }
      attack_player_ = 0;
      play_sound(ii::sound::kEnemySpawn);
    }
    if (!attack_player_) {
      special_attack_ = false;
    }
  } else if (is_on_screen()) {
    timer_--;
    if (timer_ <= 0) {
      timer_ = (sim().random(6) + 1) * kBsbTimer;
      dir_ = vec2{0} - dir_;
      reverse_ = !reverse_;
    }
    ++spawn_timer_;
    auto t = kBsbSTimer;
    t -= std::min(t, sim().alive_players() * 10);
    t /= (handle().get<ii::Health>()->is_hp_low() ? 2 : 1);
    if (spawn_timer_ >= t) {
      spawn_timer_ = 0;
      ++special_timer_;
      ii::spawn_big_follow(sim(), shape().centre, false);
      play_sound_random(ii::sound::kBossFire);
    }
    if (special_timer_ >= 8 && sim().random(4)) {
      special_timer_ = kBsbAttackTime;
      special_attack_ = true;
      special_attack_rotate_ = sim().random(2) != 0;
      attack_player_ = sim().nearest_player(shape().centre);
      play_sound(ii::sound::kBossAttack);
    }
  }

  if (special_attack_) {
    return;
  }

  move(dir_ * kBsbSpeed);
  for (std::uint32_t i = 0; i < 6; ++i) {
    shapes()[i]->rotate((i % 2 ? -1 : 1) * fixed_c::hundredth * ((1 + i) * 5));
  }
  for (std::uint32_t i = 0; i < 6; ++i) {
    shapes()[i + 6]->set_rotation(shapes()[i]->rotation());
  }
}

void BigSquareBoss::render() const {
  Boss::render();
  if ((special_timer_ / 4) % 2 && attack_player_) {
    glm::vec2 d{kBsbAttackRadius.to_float(), 0};
    if (special_attack_rotate_) {
      d = rotate(d, glm::pi<float>() / 2);
    }
    for (std::uint32_t i = 0; i < 6; ++i) {
      auto p = to_float(attack_player_->shape().centre) + d;
      ii::Polygon s{vec2{0}, 10, 4, c0, fixed_c::pi / 4, ii::shape_flag::kNone};
      s.render(sim(), p, 0);
      d = rotate(d, 2 * glm::pi<float>() / 6);
    }
  }
}

}  // namespace

namespace ii {
void spawn_big_square_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<BigSquareBoss>(sim));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(kBsbBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = ii::sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss1A});
}
}  // namespace ii