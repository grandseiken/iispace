#include "game/common/colour.h"
#include "game/geometry/legacy/line.h"
#include "game/geometry/legacy/ngon.h"
#include "game/geometry/node_conditional.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/render.h"

namespace ii::legacy {
namespace {
struct FollowHub : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 16;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 170;
  static constexpr fixed kSpeed = 1;

  static constexpr auto c = colour::hue360(240, .7f);
  template <geom::ShapeNode S>
  using fh_arrange = geom::compound<geom::translate<16, 0, S>, geom::translate<-16, 0, S>,
                                    geom::translate<0, 16, S>, geom::translate<0, -16, S>>;
  template <geom::ShapeNode S>
  using r_pi4_ngon = geom::rotate<fixed_c::pi / 4, S>;
  using fh_centre =
      r_pi4_ngon<geom::polygram<16, 4, c, 0, shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using fh_spoke = r_pi4_ngon<geom::ngon<8, 4, c>>;
  using fh_power_spoke = r_pi4_ngon<geom::polystar<8, 4, c>>;
  using shape = geom::translate_p<
      0, fh_centre,
      geom::rotate_p<1, fh_arrange<fh_spoke>, geom::if_p<2, fh_arrange<fh_power_spoke>>>>;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, power_b};
  }

  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool power_a = false;
  bool power_b = false;

  FollowHub(bool power_a, bool power_b) : power_a{power_a}, power_b{power_b} {}

  void update(Transform& transform, SimInterface& sim) {
    ++timer;
    if (timer > (power_a ? kTimer / 2 : kTimer)) {
      timer = 0;
      ++count;
      if (sim.is_on_screen(transform.centre)) {
        if (power_b) {
          spawn_chaser(sim, transform.centre);
        } else {
          spawn_follow(sim, transform.centre);
        }
        sim.emit(resolve_key::predicted()).play_random(sound::kEnemySpawn, transform.centre);
      }
    }

    auto d = sim.dimensions();
    dir = transform.centre.x < 0   ? vec2{1, 0}
        : transform.centre.x > d.x ? vec2{-1, 0}
        : transform.centre.y < 0   ? vec2{0, 1}
        : transform.centre.y > d.y ? vec2{0, -1}
        : count > 3                ? (count = 0, sim.rotate_compatibility(dir, -fixed_c::pi / 2))
                                   : dir;

    auto s = power_a ? fixed_c::hundredth * 5 + fixed_c::tenth : fixed_c::hundredth * 5;
    transform.rotate(s);
    transform.move(dir * kSpeed);
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    if (power_b) {
      spawn_big_follow(sim, transform.centre, true);
    }
    spawn_chaser(sim, transform.centre);
  }
};
DEBUG_STRUCT_TUPLE(FollowHub, timer, dir, count, power_a, power_b);

struct Shielder : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 36;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 2;

  static constexpr auto c0 = colour::hue360(150, .2f);
  static constexpr auto c1 = colour::hue360(160, .5f, .6f);
  template <geom::ShapeNode S>
  using s_arrange = geom::compound<geom::translate<24, 0, S>, geom::translate<-24, 0, S>,
                                   geom::translate<0, 24, geom::rotate<fixed_c::pi / 2, S>>,
                                   geom::translate<0, -24, geom::rotate<fixed_c::pi / 2, S>>>;
  using s_centre =
      geom::polygon_colour_p<14, 8, 2, 0, shape_flag::kDangerous | shape_flag::kVulnerable>;
  using s_shield0 = geom::polystar<8, 6, c0, 0, shape_flag::kWeakShield>;
  using s_shield1 = geom::ngon<8, 6, c1>;
  using s_spokes = geom::polystar<24, 4, c0>;
  using shape =
      standard_transform<s_spokes, s_arrange<geom::rotate_eval<geom::multiply_p<-1, 1>, s_shield0>>,
                         s_arrange<geom::rotate_eval<geom::multiply_p<-1, 1>, s_shield1>>,
                         geom::rotate_eval<geom::negate_p<1>, s_centre>>;

  std::tuple<vec2, fixed, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, power ? c1 : c0};
  }

  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool rotate = false;
  bool rdir = false;
  bool power = false;

  Shielder(bool power) : power{power} {}

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    fixed s = power ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
    transform.rotate(s);

    bool on_screen = false;
    auto dim = sim.dimensions();
    dir = transform.centre.x < 0     ? vec2{1, 0}
        : transform.centre.x > dim.x ? vec2{-1, 0}
        : transform.centre.y < 0     ? vec2{0, 1}
        : transform.centre.y > dim.y ? vec2{0, -1}
                                     : (on_screen = true, dir);

    if (!on_screen && rotate) {
      timer = 0;
      rotate = false;
    }

    fixed speed = kSpeed + (power ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - health.hp);
    if (rotate) {
      auto d = sim.rotate_compatibility(
          dir, (rdir ? 1 : -1) * (kTimer - timer) * fixed_c::pi / (2 * kTimer));
      if (!--timer) {
        rotate = false;
        dir = sim.rotate_compatibility(dir, (rdir ? 1 : -1) * fixed_c::pi / 2);
      }
      transform.move(d * speed);
    } else {
      ++timer;
      if (timer > kTimer * 2) {
        timer = kTimer;
        rotate = true;
        rdir = sim.random_bool();
      }
      if (sim.is_on_screen(transform.centre) && power && timer % kTimer == kTimer / 2) {
        spawn_boss_shot(sim, transform.centre, 3 * sim.nearest_player_direction(transform.centre),
                        colour::hue360(160, .5f, .6f));
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }
      transform.move(dir * speed);
    }
    dir = normalise(dir);
  }
};
DEBUG_STRUCT_TUPLE(Shielder, dir, timer, rotate, rdir, power);

struct Tractor : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 36;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 50;
  static constexpr fixed kSpeed = 6 * (1_fx / 10);
  static constexpr fixed kPullSpeed = 2 + 1_fx / 2;

  static constexpr auto c = colour::hue360(300, .5f, .6f);
  using t_orb = geom::polygram<12, 6, c, 0, shape_flag::kDangerous | shape_flag::kVulnerable>;
  using t_star = geom::polystar<16, 6, c>;
  using shape = standard_transform<
      geom::translate<24, 0, geom::rotate_eval<geom::multiply_p<5, 2>, t_orb>>,
      geom::translate<-24, 0, geom::rotate_eval<geom::multiply_p<-5, 2>, t_orb>>,
      geom::line<-24, 0, 24, 0, c>,
      geom::if_p<3, geom::translate<24, 0, geom::rotate_eval<geom::multiply_p<8, 2>, t_star>>,
                 geom::translate<-24, 0, geom::rotate_eval<geom::multiply_p<-8, 2>, t_star>>>>;

  std::tuple<vec2, fixed, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, spoke_r, power};
  }

  std::uint32_t timer = kTimer * 4;
  vec2 dir{0};
  bool power = false;
  bool ready = false;
  bool spinning = false;
  fixed spoke_r = 0;

  Tractor(bool power) : power{power} {}

  void update(Transform& transform, SimInterface& sim) {
    spoke_r = normalise_angle(spoke_r + fixed_c::hundredth);

    auto dim = sim.dimensions();
    if (transform.centre.x < 0) {
      dir = vec2{1, 0};
    } else if (transform.centre.x > dim.x) {
      dir = vec2{-1, 0};
    } else if (transform.centre.y < 0) {
      dir = vec2{0, 1};
    } else if (transform.centre.y > dim.y) {
      dir = vec2{0, -1};
    } else {
      ++timer;
    }

    if (!ready && !spinning) {
      transform.move(dir * kSpeed * (sim.is_on_screen(transform.centre) ? 1 : 2 + fixed_c::half));

      if (timer > kTimer * 8) {
        ready = true;
        timer = 0;
      }
    } else if (ready) {
      if (timer > kTimer) {
        ready = false;
        spinning = true;
        timer = 0;
        sim.emit(resolve_key::predicted()).play(sound::kBossFire, transform.centre);
      }
    } else if (spinning) {
      transform.rotate(fixed_c::tenth * 3);
      sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
        if (!p.is_killed) {
          p_transform.centre += normalise(transform.centre - p_transform.centre) * kPullSpeed;
        }
      });
      if (timer % (kTimer / 2) == 0 && sim.is_on_screen(transform.centre) && power) {
        spawn_boss_shot(sim, transform.centre, 4 * sim.nearest_player_direction(transform.centre),
                        colour::hue360(300, .5f, .6f));
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }

      if (timer > kTimer * 5) {
        spinning = false;
        timer = 0;
      }
    }
  }

  void render(const Transform& transform, std::vector<render::shape>& output,
              const SimInterface& sim) const {
    if (spinning) {
      std::uint32_t i = 0;
      sim.index().iterate_dispatch<Player>([&](const Player& p, const Transform& p_transform) {
        if (((timer + i++ * 4) / 4) % 2 && !p.is_killed) {
          auto s =
              render::shape::line(to_float(transform.centre), to_float(p_transform.centre),
                                  colour::hue360(300, .5f, .6f), 0.f, 1.f, 'p' + p.player_number);
          output.emplace_back(s);
        }
      });
    }
  }
};
DEBUG_STRUCT_TUPLE(Tractor, timer, dir, power, ready, spinning, spoke_r);

}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = create_ship<FollowHub>(sim, position);
  add_enemy_health<FollowHub>(h, 14);
  h.add(FollowHub{power_a, power_b});
  h.add(Enemy{.threat_value = 6u + 2 * power_a + 2 * power_b,
              .score_reward = 50u + power_a * 10 + power_b * 10});
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship<Shielder>(sim, position);
  add_enemy_health<Shielder>(h, 16);
  h.add(Shielder{power});
  h.add(Enemy{.threat_value = 8u + 2 * power, .score_reward = 60u + power * 40});
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship<Tractor>(sim, position);
  add_enemy_health<Tractor>(h, 50);
  h.add(Tractor{power});
  h.add(Enemy{.threat_value = 10u + 2 * power, .score_reward = 85u + 40 * power});
}

}  // namespace ii::legacy
