#include "game/common/colour.h"
#include "game/logic/geometry/legacy/ngon.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {

struct BigSquareBoss : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 400;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr std::uint32_t kSTimer = 80;
  static constexpr std::uint32_t kAttackTime = 90;
  static constexpr fixed kSpeed = 2 + 1_fx / 2;
  static constexpr fixed kSpecialAttackRadius = 120;
  static constexpr float kZIndex = -4.f;

  static constexpr glm::vec4 c0 = colour::hue360(270, .6f);
  static constexpr glm::vec4 c1 = colour::hue360(270, .4f);
  static constexpr glm::vec4 c2 = colour::hue360(260, .3f);

  template <fixed C, geom::ShapeNode... Nodes>
  using rotate_s = geom::rotate_eval<geom::multiply_p<C, 1>, geom::pack<Nodes...>>;
  using shape = geom::translate_p<
      0, rotate_s<1, geom::ngon<160, 4, c0>, geom::ngon<155, 4, c0>>,
      rotate_s<-2, geom::polygon<140, 4, c0, 0, shape_flag::kDangerous>, geom::ngon<135, 4, c0>>,
      rotate_s<3, geom::polygon<120, 4, c0, 0, shape_flag::kDangerous>, geom::ngon<115, 4, c0>>,
      rotate_s<-4, geom::ngon<100, 4, c0>, geom::ngon<95, 4, c1>>,
      rotate_s<5, geom::ngon<80, 4, c0>, geom::ngon<75, 4, c1>>,
      rotate_s<-6, geom::polygon<60, 4, c0, 0, shape_flag::kVulnerable>,
               geom::polygon<55, 4, c2, 0, shape_flag::kShield>>>;

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.is_legacy() ? 640 : 150;
  }

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
          d = sim.rotate_compatibility(d, fixed_c::pi / 2);
        }
        for (std::uint32_t i = 0; i < 6; ++i) {
          spawn_follow(sim, ph.get<Transform>()->centre + d, /* score */ false, fixed_c::pi / 4);
          d = sim.rotate_compatibility(d, 2 * fixed_c::pi / 6);
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

  void render(std::vector<render::shape>& output, const SimInterface& sim) const {
    using follow_shape = geom::translate_p<0, geom::rotate<fixed_c::pi / 4, geom::ngon<10, 4, c0>>>;
    if ((special_timer / 4) % 2 && attack_player) {
      vec2 d{kSpecialAttackRadius, 0};
      if (special_attack_rotate) {
        d = rotate(d, fixed_c::pi / 2);
      }
      for (std::uint32_t i = 0; i < 6; ++i) {
        auto v = sim.index().get(*attack_player)->get<Transform>()->centre + d;
        render_shape<follow_shape>(output, std::tuple{v});
        d = rotate(d, 2 * fixed_c::pi / 6);
      }
    }
  }
};
DEBUG_STRUCT_TUPLE(BigSquareBoss, dir, reverse, timer, spawn_timer, special_timer, special_attack,
                   special_attack_rotate, attack_player);

}  // namespace

void spawn_big_square_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = create_ship<BigSquareBoss>(
      sim, {sim.dimensions().x * fixed_c::hundredth * 75, sim.dimensions().y * 2});
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(BigSquareBoss::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &boss_on_hit<true, BigSquareBoss>,
      .on_destroy = ecs::call<&boss_on_destroy<BigSquareBoss>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss1A});
  h.add(BigSquareBoss{});
}

}  // namespace ii::legacy