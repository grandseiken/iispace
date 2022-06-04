#include "game/logic/boss/square.h"
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
}  // namespace

BigSquareBoss::BigSquareBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle)
: Boss{sim,
       {ii::kSimDimensions.x * fixed_c::hundredth * 75, ii::kSimDimensions.y * 2},
       ii::SimInterface::kBoss1A,
       kBsbBaseHp,
       players,
       cycle}
, dir_{0, -1}
, timer_{kBsbTimer * 6} {
  add_new_shape<Polygon>(vec2{0}, 160, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 140, 4, c0, 0, kDangerous);
  add_new_shape<Polygon>(vec2{0}, 120, 4, c0, 0, kDangerous);
  add_new_shape<Polygon>(vec2{0}, 100, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 80, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 60, 4, c0, 0, kVulnerable);

  add_new_shape<Polygon>(vec2{0}, 155, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 135, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 115, 4, c0, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 95, 4, c1, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 75, 4, c1, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 55, 4, c2, 0, kShield);
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
        auto* s = spawn_new<Follow>(attack_player_->shape().centre + d);
        s->shape().set_rotation(fixed_c::pi / 4);
        s->set_score(0);
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
    t /= (is_hp_low() ? 2 : 1);
    if (spawn_timer_ >= t) {
      spawn_timer_ = 0;
      ++special_timer_;
      spawn_new<BigFollow>(shape().centre, false);
      play_sound_random(ii::sound::kBossFire);
    }
    if (special_timer_ >= 8 && sim().random(4)) {
      special_timer_ = kBsbAttackTime;
      special_attack_ = true;
      special_attack_rotate_ = sim().random(2) != 0;
      attack_player_ = nearest_player();
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
      Polygon s{vec2{0}, 10, 4, c0, fixed_c::pi / 4, 0};
      s.render(sim(), p, 0);
      d = rotate(d, 2 * glm::pi<float>() / 6);
    }
  }
}

std::uint32_t BigSquareBoss::get_damage(std::uint32_t damage, bool magic) {
  return damage;
}