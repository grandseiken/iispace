#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/line.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/geometry/shapes/polyarc.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"

namespace ii::v0 {
namespace {
struct FollowHub : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 18;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 200;
  static constexpr fixed kSpeed = 15_fx / 16_fx;

  static constexpr auto c = colour_hue360(240, .7f);
  template <geom::ShapeNode S>
  using fh_arrange = geom::compound<geom::translate<18, 0, S>, geom::translate<-18, 0, S>,
                                    geom::translate<0, 18, S>, geom::translate<0, -18, S>>;
  template <geom::ShapeNode S>
  using r_pi4_ngon = geom::rotate<fixed_c::pi / 4, S>;
  using fh_centre =
      r_pi4_ngon<geom::polygram<18, 4, c, shape_flag::kDangerous | shape_flag::kVulnerable>>;

  using fh_spoke = r_pi4_ngon<geom::ngon<10, 4, c>>;
  using fh_big_spoke = r_pi4_ngon<geom::ngon<12, 4, c>>;
  using fh_chaser_spoke =
      r_pi4_ngon<geom::compound<geom::ngon<10, 4, c>, geom::polystar<10, 4, c>>>;

  using hub_shape = geom::translate_p<0, fh_centre, geom::rotate_p<1, fh_arrange<fh_spoke>>>;
  using big_hub_shape =
      geom::translate_p<0, fh_centre, geom::rotate_p<1, fh_arrange<fh_big_spoke>>>;
  using chaser_hub_shape =
      geom::translate_p<0, fh_centre, geom::rotate_p<1, fh_arrange<fh_chaser_spoke>>>;

  std::uint32_t timer = 0;
  std::uint32_t count = 0;
  vec2 dir{0};
  bool big = false;
  bool chaser = false;
  bool fast = false;

  FollowHub(bool big, bool chaser, bool fast) : big{big}, chaser{chaser}, fast{fast} {}

  void update(Transform& transform, SimInterface& sim) {
    ++timer;
    if (timer > (fast ? kTimer / 2 : kTimer)) {
      timer = 0;
      ++count;
      if (sim.is_on_screen(transform.centre)) {
        if (chaser) {
          spawn_chaser(sim, transform.centre);
        } else if (big) {
          spawn_big_follow(sim, transform.centre);
        } else {
          spawn_follow(sim, transform.centre);
        }
        sim.emit(resolve_key::predicted()).play_random(sound::kEnemySpawn, transform.centre);
      }
    }

    auto dim = sim.dimensions();
    dir = transform.centre.x < 24         ? vec2{1, 0}
        : transform.centre.x > dim.x - 24 ? vec2{-1, 0}
        : transform.centre.y < 24         ? vec2{0, 1}
        : transform.centre.y > dim.y - 24 ? vec2{0, -1}
        : count > 3 ? (count = 0, sim.rotate_compatibility(dir, -fixed_c::pi / 2))
                    : dir;

    auto s = fast ? fixed_c::hundredth * 4 + fixed_c::tenth / 2 : fixed_c::hundredth * 4;
    transform.rotate(s);
    transform.move(dir * kSpeed);
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    if (chaser) {
      spawn_big_chaser(sim, transform.centre);
    } else if (big) {
      spawn_huge_follow(sim, transform.centre);
    } else {
      spawn_big_follow(sim, transform.centre);
    }
  }
};
DEBUG_STRUCT_TUPLE(FollowHub, timer, count, dir, big, chaser, fast);

struct Shielder : ecs::component {
  // TODO: try making it chase the player, with acceleration?
  static constexpr std::uint32_t kBoundingWidth = 32;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 40;
  static constexpr fixed kSpeed = 1_fx;

  static constexpr auto c0 = colour_hue360(150, .2f);
  static constexpr auto c1 = colour_hue360(160, .5f, .6f);
  static constexpr auto c2 = glm::vec4{0.f, 0.f, .75f, 1.f};

  using centre_shape =
      geom::compound<geom::polystar<26, 12, c0>, geom::polygon<6, 12, c0>,
                     geom::polygon<20, 12, c1, shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using shield_shape = geom::rotate_p<2, geom::line<32, 0, 18, 0, c2>,
                                      geom::rotate<fixed_c::pi / 4, geom::line<-32, 0, -18, 0, c2>>,
                                      geom::polyarc<26, 16, 10, c2>,
                                      geom::polyarc<32, 16, 10, c2, shape_flag::kWeakShield>>;
  using shape = geom::translate_p<0, geom::rotate_p<1, centre_shape>, shield_shape>;

  std::tuple<vec2, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, shield_angle + fixed_c::pi / 2 - fixed_c::pi / 8};
  }

  Shielder(SimInterface& sim, bool power)
  : timer{sim.random(random_source::kGameState).uint(kTimer)}, power{power} {}
  std::uint32_t timer = 0;
  vec2 velocity{0, 0};
  vec2 spread_velocity{0};
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;
  std::vector<ecs::entity_id> closest;
  bool power = false;
  bool on_screen = false;
  fixed shield_angle = 0;

  void update(ecs::handle h, Transform& transform, const Health& health, SimInterface& sim) {
    transform.rotate(power ? fixed_c::hundredth * 8 : fixed_c::hundredth * 4);
    if (sim.is_on_screen(transform.centre)) {
      on_screen = true;
    }
    ++timer;
    if (!target || timer == kTimer / 2) {
      (target ? next_target : target) = sim.nearest_player(transform.centre).id();
    }
    if (timer >= kTimer) {
      if (next_target) {
        target = next_target;
      }
      next_target.reset();
      timer = 0;
    }

    auto max_speed = !on_screen ? kSpeed * 3_fx / 2
                                : kSpeed * (6_fx + fixed{health.max_hp - health.hp} / (5_fx / 2));
    auto target_v = max_speed *
        normalise(sim.index().get(*target)->get<Transform>()->centre - transform.centre);
    velocity = rc_smooth(velocity, target_v, 127_fx / 128);
    transform.move(velocity);

    static constexpr fixed kMaxRotationSpeed = 1_fx / 16_fx;
    auto smooth_rotate = [&](fixed& x, fixed target) {
      auto d = angle_diff(x, target);
      if (abs(d) < kMaxRotationSpeed) {
        x = target;
      } else {
        x += std::clamp(d, -kMaxRotationSpeed, kMaxRotationSpeed);
      }
    };
    smooth_rotate(shield_angle, angle(velocity));

    if ((+h.id() + sim.tick_count()) % 16 == 0) {
      thread_local std::vector<SimInterface::range_info> range_output;
      range_output.clear();
      closest.clear();
      sim.in_range(transform.centre, 128, ecs::id<Shielder>(), /* max */ 4, range_output);
      for (const auto& e : range_output) {
        closest.emplace_back(e.h.id());
      }
    }
    vec2 target_spread{0};
    for (auto id : closest) {
      if (auto eh = sim.index().get(id); id != h.id() && eh && !eh->has<Destroy>()) {
        auto d = eh->get<Transform>()->centre - transform.centre;
        auto d_sq = length_squared(d);
        if (d != vec2{0}) {
          target_spread -= d / std::max(1_fx, d_sq);
        }
      }
    }
    target_spread *= 64 * kSpeed;
    spread_velocity = rc_smooth(spread_velocity, target_spread, 15_fx / 16_fx);
    transform.move(spread_velocity);
  }
};
DEBUG_STRUCT_TUPLE(Shielder, timer, velocity, spread_velocity, target, next_target, closest, power,
                   on_screen, shield_angle);

struct Tractor : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 45;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 60;
  static constexpr fixed kSpeed = 6 * (15_fx / 160);
  static constexpr fixed kPullSpeed = 2 + 1_fx / 4;

  static constexpr auto c = colour_hue360(300, .5f, .6f);
  using t_orb = geom::polygram<16, 6, c, shape_flag::kDangerous | shape_flag::kVulnerable>;
  using t_star = geom::polystar<18, 6, c>;
  using shape = standard_transform<
      geom::translate<26, 0, geom::rotate_eval<geom::multiply_p<5, 2>, t_orb>>,
      geom::translate<-26, 0, geom::rotate_eval<geom::multiply_p<-5, 2>, t_orb>>,
      geom::line<-26, 0, 26, 0, c>,
      geom::if_p<3, geom::translate<26, 0, geom::rotate_eval<geom::multiply_p<8, 2>, t_star>>,
                 geom::translate<-26, 0, geom::rotate_eval<geom::multiply_p<-8, 2>, t_star>>>>;

  std::tuple<vec2, fixed, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, spoke_r, power};
  }

  Tractor(bool power) : power{power} {}
  std::uint32_t timer = kTimer * 4;
  vec2 dir{0};
  bool power = false;
  bool ready = false;
  bool spinning = false;
  fixed spoke_r = 0;

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
      transform.rotate(fixed_c::tenth * (2_fx + 1_fx / 2));
      sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
        if (!p.is_killed()) {
          p_transform.centre += normalise(transform.centre - p_transform.centre) * kPullSpeed;
        }
      });
      if (timer % (kTimer / 2) == 0 && sim.is_on_screen(transform.centre) && power) {
        // spawn_boss_shot(sim, transform.centre, 4 *
        // sim.nearest_player_direction(transform.centre),
        //                 colour_hue360(300, .5f, .6f));
        // sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
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
        if (((timer + i++ * 4) / 4) % 2 && !p.is_killed()) {
          auto s = render::shape::line(to_float(transform.centre), to_float(p_transform.centre),
                                       colour_hue360(300, .5f, .6f));
          s.disable_trail = true;
          output.emplace_back(s);
        }
      });
    }
  }
};
DEBUG_STRUCT_TUPLE(Tractor, timer, dir, power, ready, spinning, spoke_r);

}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::hub_shape>(h, 14);
  h.add(FollowHub{false, false, fast});
  h.add(Enemy{.threat_value = 6u + 4u * fast});
}

void spawn_big_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::big_hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::big_hub_shape>(h, 14);
  h.add(FollowHub{true, false, fast});
  h.add(Enemy{.threat_value = 12u + 8u * fast});
}

void spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::chaser_hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::chaser_hub_shape>(h, 14);
  h.add(FollowHub{false, true, fast});
  h.add(Enemy{.threat_value = 10u + 6u * fast});
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default<Shielder>(sim, position);
  add_enemy_health<Shielder>(h, 20);
  h.add(Shielder{sim, power});
  h.add(Enemy{.threat_value = 8u + 2u * power});
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default<Tractor>(sim, position);
  add_enemy_health<Tractor>(h, 42);
  h.add(Tractor{power});
  h.add(Enemy{.threat_value = 10u + 4u * power});
}

}  // namespace ii::v0
