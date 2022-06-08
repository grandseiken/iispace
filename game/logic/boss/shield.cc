#include "game/logic/boss/boss_internal.h"
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

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle);

  void update() override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;

private:
  std::uint32_t timer_ = 0;
  std::uint32_t count_ = 0;
  std::uint32_t unshielded_ = 0;
  std::uint32_t attack_ = 0;
  bool side_ = false;
  vec2 attack_dir_{0};
  bool shot_alternate_ = false;
};

ShieldBombBoss::ShieldBombBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle)
: Boss{sim,
       {-ii::kSimDimensions.x / 2, ii::kSimDimensions.y / 2},
       ii::SimInterface::kBoss1B,
       kSbbBaseHp,
       players,
       cycle} {
  add_new_shape<ii::Polygon>(vec2{0}, 48, 8, c0, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);

  for (std::uint32_t i = 0; i < 16; ++i) {
    vec2 a = rotate(vec2{120, 0}, i * fixed_c::pi / 8);
    vec2 b = rotate(vec2{80, 0}, i * fixed_c::pi / 8);

    add_new_shape<ii::Line>(vec2{0}, a, b, c2, 0);
  }

  add_new_shape<ii::Polygon>(vec2{0}, 130, 16, c1, 0,
                             ii::shape_flag::kVulnShield | ii::shape_flag::kDangerous);
  add_new_shape<ii::Polygon>(vec2{0}, 125, 16, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 120, 16, c1, 0);

  add_new_shape<ii::Polygon>(vec2{0}, 42, 16, glm::vec4{0.f}, 0, ii::shape_flag::kShield);

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
      shapes()[0]->category = ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable;
      shapes()[17]->category = ii::shape_flag::kDangerous | ii::shape_flag::kVulnShield;

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
    ii::spawn_boss_shot(sim(), shape().centre, d);
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
        if (sim().count_ships(ii::ship_flag::kPowerup) < 5) {
          ii::spawn_powerup(sim(), shape().centre, ii::powerup_type::kBomb);
        }
      }
    }

    if (!sim().random(3)) {
      side_ = !side_;
    }

    if (sim().random(2)) {
      auto d = rotate(vec2{5, 0}, shape().rotation());
      for (std::uint32_t i = 0; i < 12; ++i) {
        ii::spawn_boss_shot(sim(), shape().centre, d);
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
    shapes()[0]->category = ii::shape_flag::kVulnerable | ii::shape_flag::kDangerous;
    shapes()[17]->category = ii::shape_flag::kNone;
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
}  // namespace

namespace ii {
void spawn_shield_bomb_boss(SimInterface& sim, std::uint32_t players, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<ShieldBombBoss>(sim, players, cycle));
  h.add(legacy_collision(/* bounding width */ 640, h));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward = calculate_boss_score(SimInterface::kBoss1B, players, cycle)});
}
}  // namespace ii