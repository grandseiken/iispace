#include "game/common/colour.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/ngon.h"
#include "game/logic/v0/boss/boss.h"
#include "game/logic/v0/boss/boss_template.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

// TODO: many general boss todos.
// TODO: render boss health bar.
// TODO: better bomb effect when hit.
// TODO: change direction/rotation less suddenly.
// TODO: cool spawn effects; spawn with physics velocity.
// TODO: cooler special attack effect.
// TODO: special target more players?
struct SquareBoss : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 4000;
  static constexpr std::uint32_t kBiomeHp = 1500;
  static constexpr std::uint32_t kTimer = 120;
  static constexpr std::uint32_t kSpawnTimer = 100;
  static constexpr std::uint32_t kSpecialAttackTime = 110;
  static constexpr fixed kSpeed = 2 + 1_fx / 4;
  static constexpr fixed kSpecialAttackRadius = 140;
  static constexpr fixed kBoundingWidth = 200;
  static constexpr float kZIndex = -4.f;

  static constexpr auto z = colour::kZEnemyBoss;
  static constexpr auto c1 = colour::kSolarizedDarkCyan;
  static constexpr auto c0 = colour::kNewPurple;
  static constexpr auto outline = geom::nline(colour::kOutline, colour::kZOutline, 2.5f);
  static constexpr auto m_outline = geom::nline(colour::kOutline, colour::kZOutline, -2.5f);
  template <std::uint32_t T>
  static constexpr auto c_mix = colour::perceptual_mix(c0, c1, T / 5.f);

  template <fixed C, geom::ShapeNode... Nodes>
  using rotate_s = geom::rotate_eval<geom::multiply_p<C, 1>, geom::pack<Nodes...>>;
  template <fixed R, std::uint32_t T, render::flag RFlags>
  using ngon = geom::compound<
      geom::ngon<geom::nd2(50 + 25 * R, 50, 4), geom::nline(c_mix<T>, c1, z, 2.f),
                 geom::sfill(colour::alpha(c_mix<T>, colour::kFillAlpha2),
                             colour::alpha(c1, colour::kFillAlpha2), z),
                 RFlags>,
      geom::ngon<geom::nd(44 + 25 * R, 4), geom::nline(c_mix<T>, z, 4.f), geom::sfill(), RFlags>,
      geom::ngon<geom::nd(52 + 25 * R, 4), outline>, geom::ngon<geom::nd(48, 4), m_outline>>;
  template <fixed C, fixed R, std::uint32_t T, render::flag RFlags = render::flag::kNone>
  using rotate_ngon = rotate_s<C, ngon<R, T, RFlags>>;
  template <fixed C, fixed R, std::uint32_t T, shape_flag Flags,
            render::flag RFlags = render::flag::kNone>
  using rotate_ngon_c =
      rotate_s<C, ngon<R, T, RFlags>,
               geom::ngon_collider<geom::nd2(50 + 25 * R, 75 - 10 * R, 4), Flags>>;
  using shape = geom::translate_p<
      0, rotate_ngon_c<1, 6, 0, shape_flag::kBombVulnerable, render::flag::kNoFlash>,
      rotate_ngon<3_fx / 2, 5, 1, render::flag::kNoFlash>,
      rotate_ngon_c<2, 4, 2, shape_flag::kDangerous, render::flag::kNoFlash>,
      rotate_ngon<5_fx / 2, 3, 3>, rotate_ngon_c<3, 2, 4, shape_flag::kVulnerable>,
      rotate_ngon_c<7_fx / 2, 1, 5, shape_flag::kShield>>;

  vec2 dir{0, -1};
  bool reverse = false;
  std::uint32_t timer = kTimer * 6;
  std::uint32_t spawn_timer = 0;
  struct special_t {
    ecs::entity_id player{0};
    std::uint32_t timer = 0;
    bool rotate = false;
  };
  std::uint32_t special_counter = 0;
  std::optional<special_t> special_attack;

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    auto d = sim.dimensions();
    if (transform.centre.y <= d.y / 4 && dir.y <= -1) {
      dir = {reverse ? 1 : -1, 0};
    }
    if (transform.centre.x <= d.x / 5 && dir.x <= -1) {
      dir = {0, reverse ? -1 : 1};
    }
    if (transform.centre.y >= 3 * d.y / 4 && dir.y >= 1) {
      dir = {reverse ? -1 : 1, 0};
    }
    if (transform.centre.x >= 4 * d.x / 5 && dir.x >= 1) {
      dir = {0, reverse ? 1 : -1};
    }

    if (special_attack) {
      special_attack->timer && --special_attack->timer;
      auto ph = *sim.index().get(special_attack->player);
      if (ph.get<Player>()->is_killed) {
        special_attack.reset();
      } else if (!special_attack->timer) {
        vec2 d{kSpecialAttackRadius, 0};
        if (special_attack->rotate) {
          d = sim.rotate_compatibility(d, pi<fixed> / 2);
        }
        for (std::uint32_t i = 0; i < 6; ++i) {
          spawn_follow(sim, ph.get<Transform>()->centre + d, std::nullopt, /* drop */ false);
          d = sim.rotate_compatibility(d, 2 * pi<fixed> / 6);
        }
        sim.emit(resolve_key::predicted()).play(sound::kEnemySpawn, ph.get<Transform>()->centre);
        special_attack.reset();
      }
    } else if (sim.is_on_screen(transform.centre)) {
      timer && --timer;
      if (!timer) {
        timer = (sim.random(6) + 1) * kTimer;
        dir = -dir;
        reverse = !reverse;
      }
      ++spawn_timer;
      auto t = (kSpawnTimer - std::min(kSpawnTimer, sim.alive_players() * 10)) /
          (health.is_hp_low() ? 2 : 1);
      if (spawn_timer >= t) {
        spawn_timer = 0;
        ++special_counter;
        spawn_big_follow(sim, transform.centre, std::nullopt, /* drop */ false);
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }
      if (special_counter >= 8 && sim.random(4)) {
        auto player = sim.nearest_player(transform.centre);
        special_counter = 0;
        special_attack = {
            {.player = player.id(), .timer = kSpecialAttackTime, .rotate = sim.random_bool()}};
        sim.emit(resolve_key::predicted())
            .play(sound::kBossAttack, player.get<Transform>()->centre);
      }
    }

    if (special_attack) {
      return;
    }
    transform.move(dir * kSpeed);
    transform.rotate(1_fx / 50);
  }

  void render(std::vector<render::shape>& output, const SimInterface& sim) const {
    static constexpr std::uint32_t kSmallWidth = 11;
    static constexpr auto c = colour::kNewPurple;
    static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);
    static constexpr float z = colour::kZEnemySmall;
    using small_follow_shape = standard_transform<
        geom::ngon<geom::nd(kSmallWidth + 2, 4), outline>,
        geom::ngon<geom::nd(kSmallWidth, 4), geom::nline(c, z, 1.5f), geom::sfill(cf, z)>>;
    if (special_attack && (special_attack->timer / 4) % 2) {
      auto ph = sim.index().get(special_attack->player);
      if (ph && !ph->get<Player>()->is_killed) {
        vec2 d{kSpecialAttackRadius, 0};
        if (special_attack->rotate) {
          d = rotate(d, pi<fixed> / 2);
        }
        for (std::uint32_t i = 0; i < 6; ++i) {
          auto v = ph->get<Transform>()->centre + d;
          render_shape<small_follow_shape>(output, std::tuple{v, pi<fixed>});
          d = rotate(d, 2 * pi<fixed> / 6);
        }
      }
    }
  }
};
DEBUG_STRUCT_TUPLE(SquareBoss::special_t, player, timer, rotate);
DEBUG_STRUCT_TUPLE(SquareBoss, dir, reverse, timer, spawn_timer, special_counter, special_attack);

}  // namespace

void spawn_biome0_square_boss(SimInterface& sim, std::uint32_t biome_index) {
  auto h =
      create_ship_default<SquareBoss>(sim, {sim.dimensions().x * 3_fx / 4, sim.dimensions().y * 2});
  add_boss_data<SquareBoss>(h, SquareBoss::kBaseHp + SquareBoss::kBiomeHp * biome_index);
  h.add(SquareBoss{});
}

}  // namespace ii::v0