#include "game/common/colour.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {
using namespace geom2;

struct BigSquareBoss : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 400;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr std::uint32_t kSTimer = 80;
  static constexpr std::uint32_t kAttackTime = 90;
  static constexpr fixed kSpeed = 2 + 1_fx / 2;
  static constexpr fixed kSpecialAttackRadius = 120;
  static constexpr float kZIndex = -4.f;

  static constexpr cvec4 c0 = colour::hue360(270, .6f);
  static constexpr cvec4 c1 = colour::hue360(270, .4f);
  static constexpr cvec4 c2 = colour::hue360(260, .3f);
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate{key{'v'}});
    for (std::uint32_t i = 0; i < 6; ++i) {
      auto& r = n.add(rotate{multiply(fixed{i + 1} * (i % 2 ? -1_fx : 1_fx), key{'r'})});
      auto d = 160 - 20_fx * i;
      r.add(ngon{.dimensions = nd(d, 4), .line = {.colour0 = c0}});
      r.add(ngon{.dimensions = nd(d - 5, 4), .line = {.colour0 = i < 3 ? c0 : i < 5 ? c1 : c2}});

      if (i == 1 || i == 2) {
        r.add(ball_collider{.dimensions = bd(d), .flags = shape_flag::kDangerous});
      }
      if (i == 5) {
        r.add(ball_collider{.dimensions = bd(d), .flags = shape_flag::kVulnerable});
        r.add(ball_collider{.dimensions = bd(d - 5), .flags = shape_flag::kShield});
      }
    }
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
  }

  static constexpr fixed bounding_width(bool is_legacy) { return is_legacy ? 640 : 150; }

  vec2 dir{0, -1};
  bool reverse = false;
  std::uint32_t timer = kTimer * 6;
  std::uint32_t spawn_timer = 0;
  std::uint32_t special_timer = 0;
  bool special_attack = false;
  bool special_attack_rotate = false;
  std::optional<ecs::entity_id> attack_player;

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    auto d = sim.dimensions();
    if (transform.centre.y < d.y * fixed_c::hundredth * 25 && dir.y == -1) {
      dir = {reverse ? 1 : -1, 0};
    }
    if (transform.centre.x < d.x * fixed_c::hundredth * 25 && dir.x == -1) {
      dir = {0, reverse ? -1 : 1};
    }
    if (transform.centre.y > d.y * fixed_c::hundredth * 75 && dir.y == 1) {
      dir = {reverse ? -1 : 1, 0};
    }
    if (transform.centre.x > d.x * fixed_c::hundredth * 75 && dir.x == 1) {
      dir = {0, reverse ? 1 : -1};
    }

    if (special_attack) {
      special_timer--;
      auto ph = *sim.index().get(*attack_player);
      if (ph.get<Player>()->is_killed) {
        special_timer = 0;
        attack_player.reset();
      } else if (!special_timer) {
        vec2 d(kSpecialAttackRadius, 0);
        if (special_attack_rotate) {
          d = sim.rotate_compatibility(d, pi<fixed> / 2);
        }
        for (std::uint32_t i = 0; i < 6; ++i) {
          spawn_follow(sim, ph.get<Transform>()->centre + d, /* score */ false, pi<fixed> / 4);
          d = sim.rotate_compatibility(d, 2 * pi<fixed> / 6);
        }
        attack_player.reset();
        sim.emit(resolve_key::predicted()).play(sound::kEnemySpawn, ph.get<Transform>()->centre);
      }
      if (!attack_player) {
        special_attack = false;
      }
    } else if (sim.is_on_screen(transform.centre)) {
      timer--;
      if (timer <= 0) {
        timer = (sim.random(6) + 1) * kTimer;
        dir = -dir;
        reverse = !reverse;
      }
      ++spawn_timer;
      auto t =
          (kSTimer - std::min(kSTimer, sim.alive_players() * 10)) / (health.is_hp_low() ? 2 : 1);
      if (spawn_timer >= t) {
        spawn_timer = 0;
        ++special_timer;
        spawn_big_follow(sim, transform.centre, false);
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }
      if (special_timer >= 8 && sim.random(4)) {
        special_timer = kAttackTime;
        special_attack = true;
        special_attack_rotate = sim.random_bool();
        attack_player = sim.nearest_player(transform.centre).id();
        sim.emit(resolve_key::predicted())
            .play(sound::kBossAttack, sim.index().get(*attack_player)->get<Transform>()->centre);
      }
    }

    if (special_attack) {
      return;
    }

    transform.move(dir * kSpeed);
    transform.rotate(5 * fixed_c::hundredth);
  }

  static void construct_follow_attack_shape(node& root) {
    root.add(translate_rotate{key{'v'}, pi<fixed> / 4})
        .add(ngon{.dimensions = nd(10, 4), .line = {.colour0 = c0}});
  }

  void render(std::vector<render::shape>& output, const SimInterface& sim) const {
    if ((special_timer / 4) % 2 && attack_player) {
      vec2 d{kSpecialAttackRadius, 0};
      if (special_attack_rotate) {
        d = ::rotate(d, pi<fixed> / 2);
      }
      for (std::uint32_t i = 0; i < 6; ++i) {
        auto v = sim.index().get(*attack_player)->get<Transform>()->centre + d;
        auto& r = resolve_shape2<&construct_follow_attack_shape>(
            sim, [&](parameter_set& parameters) { parameters.add(key{'v'}, v); });
        render_shape(output, r);
        d = ::rotate(d, 2 * pi<fixed> / 6);
      }
    }
  }
};
DEBUG_STRUCT_TUPLE(BigSquareBoss, dir, reverse, timer, spawn_timer, special_timer, special_attack,
                   special_attack_rotate, attack_player);

}  // namespace

void spawn_big_square_boss(SimInterface& sim, std::uint32_t cycle) {
  using legacy_shape =
      shape_definition_with_width<BigSquareBoss, BigSquareBoss::bounding_width(true)>;
  using shape = shape_definition_with_width<BigSquareBoss, BigSquareBoss::bounding_width(false)>;

  vec2 position{sim.dimensions().x * fixed_c::hundredth * 75, sim.dimensions().y * 2};
  auto h = sim.is_legacy() ? create_ship2<BigSquareBoss, legacy_shape>(sim, position)
                           : create_ship2<BigSquareBoss, shape>(sim, position);
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(BigSquareBoss::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &boss_on_hit2<true, BigSquareBoss, shape>,
      .on_destroy = ecs::call<&boss_on_destroy2<BigSquareBoss, shape>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss1A});
  h.add(BigSquareBoss{});
}

}  // namespace ii::legacy