#include "game/common/colour.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/player/powerup.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {
using namespace geom;

struct ShieldBombBoss : ecs::component {
  static constexpr std::uint32_t kBaseHp = 320;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr std::uint32_t kUnshieldTime = 300;
  static constexpr std::uint32_t kAttackTime = 80;
  static constexpr fixed kSpeed = 1;
  static constexpr float kZIndex = -4.f;

  static constexpr cvec4 c0 = colour::hue360(150, .4f, .5f);
  static constexpr cvec4 c1 = colour::hue(0.f, .8f, 0.f);
  static constexpr cvec4 c2 = colour::hue(0.f, .6f, 0.f);
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable |
      shape_flag::kWeakShield | shape_flag::kShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    auto& centre = n.add(rotate{multiply(2_fx, key{'r'})});
    centre.add(
        ngon{.dimensions = nd(48, 8), .style = ngon_style::kPolygram, .line = {.colour0 = c0}});
    centre.add(ball_collider{.dimensions = bd(48),
                             .flags = shape_flag::kDangerous | shape_flag::kVulnerable});

    n.add(ngon{
        .dimensions = nd(130, 16), .line = {.colour0 = key{'c'}}, .flags = render::flag::kNoFlash});
    n.add(enable{key{'s'}})
        .add(ball_collider{.dimensions = bd(130),
                           .flags = shape_flag::kWeakShield | shape_flag::kDangerous});
    for (std::uint32_t i = 0; i < 16; ++i) {
      n.add(line{.a = ::rotate(vec2{80, 0}, i * pi<fixed> / 8),
                 .b = ::rotate(vec2{120, 0}, i * pi<fixed> / 8),
                 .style = {.colour0 = key{'C'}},
                 .flags = render::flag::kNoFlash});
    }

    n.add(ngon{
        .dimensions = nd(125, 16), .line = {.colour0 = key{'c'}}, .flags = render::flag::kNoFlash});
    n.add(ngon{
        .dimensions = nd(120, 16), .line = {.colour0 = key{'c'}}, .flags = render::flag::kNoFlash});
    auto& r = n.add(rotate{key{'r'}});
    r.add(ball{.line = {.colour0 = key{'c'}}, .flags = render::flag::kNoFlash});
    r.add(ball_collider{.dimensions = bd(42), .flags = shape_flag::kShield});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    auto c = !unshielded ? c2 : (unshielded / 2) % 4 ? colour::kZero : colour::hue(0.f, .2f, 0.f);
    auto c_strut = !unshielded ? c1
        : (unshielded / 2) % 4 ? colour::kZero
                               : colour::hue(0.f, .4f, 0.f);

    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, c)
        .add(key{'C'}, c_strut)
        .add(key{'s'}, !unshielded);
  }

  static constexpr fixed bounding_width(bool is_legacy) { return is_legacy ? 640 : 140; }

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
          attack_dir, (kAttackTime - attack) * fixed_c::half * pi<fixed> / kAttackTime);
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
          d = sim.rotate_compatibility(d, 2 * pi<fixed> / 12);
        }
        sim.emit(resolve_key::predicted()).play(sound::kBossAttack, transform.centre);
      } else {
        attack = kAttackTime;
        attack_dir = from_polar_legacy(sim.random_fixed() * (2 * pi<fixed>), 5_fx);
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
  using legacy_shape =
      shape_definition_with_width<ShieldBombBoss, ShieldBombBoss::bounding_width(true)>;
  using shape = shape_definition_with_width<ShieldBombBoss, ShieldBombBoss::bounding_width(false)>;

  vec2 position{-sim.dimensions().x / 2, sim.dimensions().y / 2};
  auto h = sim.is_legacy() ? create_ship<ShieldBombBoss, legacy_shape>(sim, position)
                           : create_ship<ShieldBombBoss, shape>(sim, position);
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1B, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(ShieldBombBoss::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &transform_shield_bomb_boss_damage,
      .on_hit = &boss_on_hit<true, ShieldBombBoss, shape>,
      .on_destroy = ecs::call<&boss_on_destroy<ShieldBombBoss, shape>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss1B});
  h.add(ShieldBombBoss{});
}

}  // namespace ii::legacy