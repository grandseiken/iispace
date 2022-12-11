#include "game/common/colour.h"
#include "game/geometry/legacy/line.h"
#include "game/geometry/legacy/ngon.h"
#include "game/geometry/node_for_each.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/player/powerup.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {

struct ShieldBombBoss : ecs::component {
  static constexpr std::uint32_t kBaseHp = 320;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr std::uint32_t kUnshieldTime = 300;
  static constexpr std::uint32_t kAttackTime = 80;
  static constexpr fixed kSpeed = 1;
  static constexpr float kZIndex = -4.f;

  static constexpr glm::vec4 c0 = colour::hue360(150, .4f, .5f);
  static constexpr glm::vec4 c1 = colour::hue(0.f, .8f, 0.f);
  static constexpr glm::vec4 c2 = colour::hue(0.f, .6f, 0.f);

  template <fixed I>
  using strut_shape =
      geom::line_eval<geom::constant<rotate(vec2{80, 0}, I* fixed_c::pi / 8)>,
                      geom::constant<rotate(vec2{120, 0}, I* fixed_c::pi / 8)>, geom::parameter<3>>;

  using centre_shape =
      geom::polygram<48, 8, c0, 0, shape_flag::kDangerous | shape_flag::kVulnerable>;
  using shield_shape = geom::ngon_eval<
      geom::constant<130u>, geom::constant<16u>, geom::parameter<4>,
      geom::constant<geom::ngon_style::kPolygon>, geom::constant<0>,
      geom::ternary_p<2, geom::constant<shape_flag::kNone>,
                      geom::constant<shape_flag::kWeakShield | shape_flag::kDangerous>>>;

  using shape = standard_transform<
      geom::rotate_eval<geom::multiply_p<2, 1>, centre_shape>,
      geom::for_each<fixed, 0, 16, strut_shape>, shield_shape, geom::ngon_colour_p<125, 16, 4>,
      geom::ngon_colour_p<120, 16, 4>,
      geom::rotate_p<1, geom::polygon<42, 16, glm::vec4{0.f}, 0, shape_flag::kShield>>>;

  std::tuple<vec2, fixed, bool, glm::vec4, glm::vec4>
  shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, unshielded != 0,
            !unshielded                ? c2
                : (unshielded / 2) % 4 ? glm::vec4{0.f}
                                       : colour::hue(0.f, .2f, 0.f),
            !unshielded                ? c1
                : (unshielded / 2) % 4 ? glm::vec4{0.f}
                                       : colour::hue(0.f, .4f, 0.f)};
  }

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.is_legacy() ? 640 : 140;
  }

  std::uint32_t timer = 0;
  std::uint32_t count = 0;
  std::uint32_t unshielded = 0;
  std::uint32_t attack = 0;
  vec2 attack_dir{0};
  bool side = false;
  bool shot_alternate = false;

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    if (!side && transform.centre.x < sim.dimensions().x * fixed_c::tenth * 6) {
      transform.move(vec2{1, 0} * kSpeed);
    } else if (side && transform.centre.x > sim.dimensions().x * fixed_c::tenth * 4) {
      transform.move(vec2{-1, 0} * kSpeed);
    }
    transform.rotate(-fixed_c::hundredth * 2);

    if (!sim.is_on_screen(transform.centre)) {
      return;
    }

    if (health.is_hp_low()) {
      ++timer;
    }

    if (unshielded) {
      ++timer;
      --unshielded;
    }

    if (attack) {
      auto d = sim.rotate_compatibility(
          attack_dir, (kAttackTime - attack) * fixed_c::half * fixed_c::pi / kAttackTime);
      spawn_boss_shot(sim, transform.centre, d);
      attack--;
      sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
    }

    ++timer;
    if (timer > kTimer) {
      ++count;
      timer = 0;

      if (count >= 4 && (!sim.random(4) || count >= 8)) {
        count = 0;
        if (!unshielded) {
          if (sim.index().count<PowerupTag>() < 5) {
            spawn_powerup(sim, transform.centre, powerup_type::kBomb);
          }
        }
      }

      if (!sim.random(3)) {
        side = !side;
      }

      if (sim.random_bool()) {
        auto d = sim.rotate_compatibility(vec2{5, 0}, transform.rotation);
        for (std::uint32_t i = 0; i < 12; ++i) {
          spawn_boss_shot(sim, transform.centre, d);
          d = sim.rotate_compatibility(d, 2 * fixed_c::pi / 12);
        }
        sim.emit(resolve_key::predicted()).play(sound::kBossAttack, transform.centre);
      } else {
        attack = kAttackTime;
        attack_dir = from_polar(sim.random_fixed() * (2 * fixed_c::pi), 5_fx);
      }
    }
  }

  std::pair<bool, std::uint32_t> get_damage(std::uint32_t damage, damage_type type) {
    if (unshielded) {
      return {false, damage};
    }
    if (type == damage_type::kBomb && !unshielded) {
      unshielded = kUnshieldTime;
    }
    if (type != damage_type::kPenetrating) {
      return {false, 0};
    }
    shot_alternate = !shot_alternate;
    return {shot_alternate, damage};
  }
};
DEBUG_STRUCT_TUPLE(ShieldBombBoss, timer, count, unshielded, attack, attack_dir, side,
                   shot_alternate);

std::uint32_t transform_shield_bomb_boss_damage(ecs::handle h, SimInterface& sim, damage_type type,
                                                std::uint32_t damage) {
  // TODO: weird function.
  auto [undo, d] = ecs::call<&ShieldBombBoss::get_damage>(h, damage, type);
  d = scale_boss_damage(h, sim, type, d);
  if (undo) {
    h.get<Health>()->hp += d;
  }
  return d;
}
}  // namespace

void spawn_shield_bomb_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = create_ship<ShieldBombBoss>(sim, {-sim.dimensions().x / 2, sim.dimensions().y / 2});
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1B, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(ShieldBombBoss::kBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 1,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &transform_shield_bomb_boss_damage,
      .on_hit = &boss_on_hit<true, ShieldBombBoss>,
      .on_destroy = ecs::call<&boss_on_destroy<ShieldBombBoss>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss1B});
  h.add(ShieldBombBoss{});
}

}  // namespace ii::legacy