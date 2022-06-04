#include "game/logic/boss/shield.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"

namespace {
const std::uint32_t kSbbBaseHp = 320;
const std::uint32_t kSbbTimer = 100;
const std::uint32_t kSbbUnshieldTime = 300;
const std::uint32_t kSbbAttackTime = 80;
const fixed kSbbSpeed = 1;

const glm::vec4 c0 = colour_hue360(150, .4f, .5f);
const glm::vec4 c1 = colour_hue(0.f, .8f, 0.f);
const glm::vec4 c2 = colour_hue(0.f, .6f, 0.f);
}  // namespace

ShieldBombBoss::ShieldBombBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle)
: Boss{sim,
       {-ii::kSimDimensions.x / 2, ii::kSimDimensions.y / 2},
       ii::SimInterface::kBoss1B,
       kSbbBaseHp,
       players,
       cycle} {
  add_new_shape<ii::Polygon>(vec2{0}, 48, 8, c0, 0, kDangerous | kVulnerable,
                             ii::Polygon::T::kPolygram);

  for (std::uint32_t i = 0; i < 16; ++i) {
    vec2 a = rotate(vec2{120, 0}, i * fixed_c::pi / 8);
    vec2 b = rotate(vec2{80, 0}, i * fixed_c::pi / 8);

    add_new_shape<ii::Line>(vec2{0}, a, b, c2, 0);
  }

  add_new_shape<ii::Polygon>(vec2{0}, 130, 16, c1, 0, kVulnShield | kDangerous);
  add_new_shape<ii::Polygon>(vec2{0}, 125, 16, c1, 0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 120, 16, c1, 0, 0);

  add_new_shape<ii::Polygon>(vec2{0}, 42, 16, glm::vec4{0.f}, 0, kShield);

  set_ignore_damage_colour_index(1);
}

void ShieldBombBoss::update() {
  if (!side_ && shape().centre.x < ii::kSimDimensions.x * fixed_c::tenth * 6) {
    move(vec2{1, 0} * kSbbSpeed);
  } else if (side_ && shape().centre.x > ii::kSimDimensions.x * fixed_c::tenth * 4) {
    move(vec2{-1, 0} * kSbbSpeed);
  }

  shape().rotate(-fixed_c::hundredth * 2);
  shapes()[0]->rotate(-fixed_c::hundredth * 4);
  shapes()[20]->set_rotation(shapes()[0]->rotation());

  if (!is_on_screen()) {
    return;
  }

  if (is_hp_low()) {
    ++timer_;
  }

  if (unshielded_) {
    ++timer_;

    unshielded_--;
    for (std::size_t i = 0; i < 3; ++i) {
      shapes()[i + 17]->colour = (unshielded_ / 2) % 4 ? glm::vec4{0.f} : colour_hue(0.f, .4f, 0.f);
    }
    for (std::size_t i = 0; i < 16; ++i) {
      shapes()[i + 1]->colour = (unshielded_ / 2) % 4 ? glm::vec4{0.f} : colour_hue(0.f, .2f, 0.f);
    }

    if (!unshielded_) {
      shapes()[0]->category = kDangerous | kVulnerable;
      shapes()[17]->category = kDangerous | kVulnShield;

      for (std::size_t i = 0; i < 3; ++i) {
        shapes()[i + 17]->colour = c1;
      }
      for (std::size_t i = 0; i < 16; ++i) {
        shapes()[i + 1]->colour = c2;
      }
    }
  }

  if (attack_) {
    auto d = rotate(attack_dir_,
                    (kSbbAttackTime - attack_) * fixed_c::half * fixed_c::pi / kSbbAttackTime);
    spawn_new<BossShot>(shape().centre, d);
    attack_--;
    play_sound_random(ii::sound::kBossFire);
  }

  ++timer_;
  if (timer_ > kSbbTimer) {
    ++count_;
    timer_ = 0;

    if (count_ >= 4 && (!sim().random(4) || count_ >= 8)) {
      count_ = 0;
      if (!unshielded_) {
        if (sim().all_ships(kShipPowerup).size() < 5) {
          spawn_new<Powerup>(shape().centre, Powerup::type::kBomb);
        }
      }
    }

    if (!sim().random(3)) {
      side_ = !side_;
    }

    if (sim().random(2)) {
      auto d = rotate(vec2{5, 0}, shape().rotation());
      for (std::uint32_t i = 0; i < 12; ++i) {
        spawn_new<BossShot>(shape().centre, d);
        d = rotate(d, 2 * fixed_c::pi / 12);
      }
      play_sound(ii::sound::kBossAttack);
    } else {
      attack_ = kSbbAttackTime;
      attack_dir_ = from_polar(sim().random_fixed() * (2 * fixed_c::pi), 5_fx);
    }
  }
}

std::uint32_t ShieldBombBoss::get_damage(std::uint32_t damage, bool magic) {
  if (unshielded_) {
    return damage;
  }
  if (damage >= Player::kBombDamage && !unshielded_) {
    unshielded_ = kSbbUnshieldTime;
    shapes()[0]->category = kVulnerable | kDangerous;
    shapes()[17]->category = 0;
  }
  if (!magic) {
    return 0;
  }
  shot_alternate_ = !shot_alternate_;
  if (shot_alternate_) {
    restore_hp(60 / (1 + (sim().get_lives() ? sim().player_count() : sim().alive_players())));
  }
  return damage;
}