#include "game/common/colour.h"
#include "game/geometry/node_conditional.h"
#include "game/geometry/shapes/ball.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/line.h"
#include "game/geometry/shapes/ngon.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"

namespace ii::v0 {
namespace {
struct FollowHub : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 18;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 200;
  static constexpr fixed kSpeed = 15_fx / 16_fx;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c = colour::kSolarizedDarkBlue;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);
  static constexpr auto outline = geom::nline(colour::kOutline, colour::kZOutline, 2.f);
  template <geom::ShapeNode S>
  using fh_arrange = geom::compound<geom::translate<18, 0, S>, geom::translate<-18, 0, S>,
                                    geom::translate<0, 18, S>, geom::translate<0, -18, S>>;
  template <geom::ShapeNode... S>
  using r_pi4_ngon = geom::rotate<pi<fixed> / 4, S...>;
  using fh_centre =
      r_pi4_ngon<geom::ngon<geom::nd(20, 4), outline>,
                 geom::ngon_with_collider<
                     geom::nd(18, 4), geom::nline(geom::ngon_style::kPolygram, c, z),
                     geom::sfill(cf, z), shape_flag::kDangerous | shape_flag::kVulnerable>>;

  using fh_spoke = r_pi4_ngon<geom::ngon<geom::nd(12, 4), outline>,
                              geom::ngon<geom::nd(10, 4), geom::nline(c, z), geom::sfill(cf, z)>>;
  using fh_big_spoke =
      geom::compound<fh_spoke, r_pi4_ngon<geom::ngon<geom::nd(6, 4), geom::nline(c, z, 2.f)>>>;
  using fh_chaser_spoke =
      r_pi4_ngon<geom::ngon<geom::nd(12, 4), outline>,
                 geom::ngon<geom::nd(10, 4), geom::nline(geom::ngon_style::kPolygram, c, z),
                            geom::sfill(cf, z)>>;

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
          spawn_chaser(sim, transform.centre, /* drop */ false);
        } else if (big) {
          spawn_big_follow(sim, transform.centre, std::nullopt, /* drop */ false);
        } else {
          spawn_follow(sim, transform.centre, std::nullopt, /* drop */ false);
        }
        sim.emit(resolve_key::predicted()).play_random(sound::kEnemySpawn, transform.centre);
      }
    }

    auto dim = sim.dimensions();
    dir = transform.centre.x < 24         ? vec2{1, 0}
        : transform.centre.x > dim.x - 24 ? vec2{-1, 0}
        : transform.centre.y < 24         ? vec2{0, 1}
        : transform.centre.y > dim.y - 24 ? vec2{0, -1}
        : count > 3 ? (count = 0, sim.rotate_compatibility(dir, -pi<fixed> / 2))
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
      spawn_big_chaser(sim, transform.centre, /* drop */ false);
    } else if (big) {
      spawn_huge_follow(sim, transform.centre, std::nullopt, /* drop */ false);
    } else {
      spawn_big_follow(sim, transform.centre, std::nullopt, /* drop */ false);
    }
  }
};
DEBUG_STRUCT_TUPLE(FollowHub, timer, count, dir, big, chaser, fast);

// TODO: add hard variant that... lays bombs when going fast?
struct Shielder : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 32;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 40;
  static constexpr fixed kSpeed = 1_fx;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c0 = colour::hsl_mix(colour::kSolarizedDarkCyan, colour::kNewGreen0);
  static constexpr auto c1 = colour::kWhite1;
  static constexpr auto cf = colour::alpha(c0, colour::kFillAlpha0);
  static constexpr auto outline = geom::nline(colour::kOutline, colour::kZOutline, 2.f);

  using centre_shape = geom::compound<
      geom::ngon<geom::nd(22, 12), outline>,
      geom::ngon<geom::nd2(29, 6, 12),
                 geom::nline(geom::ngon_style::kPolystar, colour::kOutline, colour::kZOutline,
                             5.f)>,
      geom::ngon<geom::nd2(27 + 1_fx / 2, 6, 12), geom::nline(geom::ngon_style::kPolystar, c0, z)>,
      geom::ngon<geom::nd(6, 12), geom::nline(c0, z)>,
      geom::ngon<geom::nd(20, 12), geom::nline(c0, z), geom::sfill(cf, z)>,
      geom::ngon_collider<geom::nd(20, 12), shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using shield_shape = geom::rotate_p<
      2, geom::line<vec2{32, 0}, vec2{18, 0}, geom::sline(c1, z)>,
      geom::rotate<pi<fixed> / 4, geom::line<vec2{-32, 0}, vec2{-18, 0}, geom::sline(c1, z)>>,
      geom::ngon<geom::nd2(27 + 1_fx / 2, 22, 16, 10), geom::nline(), geom::sfill(cf, z)>,
      geom::ngon<geom::nd2(27 + 1_fx / 2, 22, 16, 10), geom::nline(),
                 geom::sfill(colour::alpha(c1, colour::kFillAlpha0), z)>,
      geom::ngon<geom::nd(29, 16, 10), outline>,
      geom::ngon<geom::nd(30, 16, 10), geom::nline(c1, z, 1.25f)>,
      geom::ngon<geom::nd(31, 16, 10), outline>,
      geom::ngon<geom::nd(32, 16, 10), geom::nline(c1, z, 1.25f)>,
      geom::ngon<geom::nd(34, 16, 10), outline>,
      geom::ngon_collider<geom::nd(32, 16, 10), shape_flag::kWeakShield>>;
  using shape = geom::translate_p<0, geom::rotate_p<1, centre_shape>, shield_shape>;

  std::tuple<vec2, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, shield_angle + pi<fixed> / 2 - pi<fixed> / 8};
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

    auto t = fixed{health.max_hp - health.hp} / 20_fx;
    auto max_speed = !on_screen ? kSpeed * 3_fx / 2 : kSpeed * (6_fx + t);
    auto target_v = max_speed *
        normalise(sim.index().get(*target)->get<Transform>()->centre - transform.centre);
    velocity = rc_smooth(velocity, target_v, (127_fx - t / 12) / 128);
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

    vec2 target_spread = spread_closest<Shielder>(h, transform, sim, closest, 128_fx, 4u,
                                                  spread_linear_coefficient(4_fx));
    target_spread *= 64 * kSpeed;
    spread_velocity = rc_smooth(spread_velocity, target_spread, 15_fx / 16_fx);
    transform.move(spread_velocity);
  }
};
DEBUG_STRUCT_TUPLE(Shielder, timer, velocity, spread_velocity, target, next_target, closest, power,
                   on_screen, shield_angle);

// TODO: add hard variant that shoots... bounces?
struct Tractor : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 45;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 60;
  static constexpr fixed kSpeed = 6 * (15_fx / 160);
  static constexpr fixed kPullSpeed = 2 + 3_fx / 8;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c = colour::kSolarizedDarkMagenta;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);
  using t_orb = geom::compound<
      geom::ngon<geom::nd(18, 6), geom::nline(colour::kOutline, colour::kZOutline, 2.f)>,
      geom::ngon_with_collider<geom::nd(16, 6), geom::nline(geom::ngon_style::kPolygram, c, z),
                               geom::sfill(cf, z),
                               shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using t_star = geom::ngon<geom::nd(18, 6), geom::nline(geom::ngon_style::kPolystar, c, z)>;
  using shape = standard_transform<
      geom::translate<26, 0, geom::rotate_eval<geom::multiply_p<5, 2>, t_orb>>,
      geom::translate<-26, 0, geom::rotate_eval<geom::multiply_p<-5, 2>, t_orb>>,
      geom::line<vec2{-26, 0}, vec2{26, 0}, geom::sline(colour::kOutline, colour::kZOutline, 5.f)>,
      geom::line<vec2{-26, 0}, vec2{26, 0}, geom::sline(c, z)>,
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

    static constexpr auto kSpinSpeed = fixed_c::tenth * (2_fx + 1_fx / 2);
    if (!ready && !spinning) {
      transform.move(dir * kSpeed * (sim.is_on_screen(transform.centre) ? 1 : 2 + fixed_c::half));

      if (timer > kTimer * 8) {
        ready = true;
        timer = 0;
      }
    } else if (ready) {
      auto r = fixed{timer} / (kTimer + 1);
      transform.rotate(r * r * kSpinSpeed * 5_fx / 8);
      if (timer > kTimer) {
        ready = false;
        spinning = true;
        timer = 0;
        sim.emit(resolve_key::predicted()).play(sound::kBossFire, transform.centre);
      }
    } else if (spinning) {
      transform.rotate(kSpinSpeed);
      sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
        if (!p.is_killed) {
          p_transform.centre += normalise(transform.centre - p_transform.centre) * kPullSpeed;
        }
      });
      if (timer % (kTimer / 2) == 0 && sim.is_on_screen(transform.centre) && power) {
        // spawn_boss_shot(sim, transform.centre, 4 *
        // sim.nearest_player_direction(transform.centre), c);
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
      sim.index().iterate_dispatch<Player>([&](const Player& pc, const Transform& p_transform) {
        // TODO: better effect.
        if ((timer + 2 * pc.player_number) % 8 < 4 && !pc.is_killed) {
          unsigned char index = 'p' + pc.player_number;
          output.emplace_back(render::shape::line(to_float(transform.centre),
                                                  to_float(p_transform.centre), c,
                                                  colour::kZEffect0, 1.f, index));
          output.emplace_back(render::shape::line(to_float(transform.centre),
                                                  to_float(p_transform.centre), colour::kOutline,
                                                  colour::kZOutline, 5.f, index));
        }
      });
    }
  }
};
DEBUG_STRUCT_TUPLE(Tractor, timer, dir, power, ready, spinning, spoke_r);

struct ShieldHub : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 28;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 280;
  static constexpr fixed kSpeed = 3_fx / 4_fx;
  static constexpr fixed kShieldDistance = 260;
  static constexpr fixed kShieldDrawDistance = kShieldDistance + 10;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c0 = colour::hsl_mix(colour::kSolarizedDarkCyan, colour::kNewGreen0);
  static constexpr auto c1 = colour::kWhite1;
  static constexpr auto cf0 = colour::alpha(c0, colour::kFillAlpha0);
  static constexpr auto cf1 = colour::alpha(c1, colour::kFillAlpha1);
  static constexpr auto outline = geom::sline(colour::kOutline, colour::kZOutline, 2.f);

  using shape = geom::compound<
      geom::translate_p<
          0, geom::ball<geom::bd(kBoundingWidth / 3), geom::sline(c0, z)>,
          geom::rotate_eval<
              geom::multiply_p<-1_fx / 2, 1>,
              geom::compound<
                  geom::translate<-18, 0, geom::box<vec2{10, 10}, outline>,
                                  geom::box<vec2{8, 8}, geom::sline(c1, z), geom::sfill(cf1, z)>>,
                  geom::translate<18, 0, geom::box<vec2{10, 10}, outline>,
                                  geom::box<vec2{8, 8}, geom::sline(c1, z), geom::sfill(cf1, z)>>,
                  geom::box<vec2{11, 4}, geom::sline(c1, z)>>>,
          geom::rotate_eval<
              geom::multiply_p<1_fx / 2, 1>,
              geom::compound<
                  geom::translate<-18, 0, geom::box<vec2{10, 10}, outline>,
                                  geom::box<vec2{8, 8}, geom::sline(c1, z), geom::sfill(cf1, z)>>,
                  geom::translate<18, 0, geom::box<vec2{10, 10}, outline>,
                                  geom::box<vec2{8, 8}, geom::sline(c1, z), geom::sfill(cf1, z)>>,
                  geom::box<vec2{11, 4}, geom::sline(c1, z)>>>>,
      standard_transform<geom::ball<geom::bd(kBoundingWidth + 2), outline>,
                         geom::ball_with_collider<
                             geom::bd(kBoundingWidth), geom::sline(c0, z), geom::sfill(cf0, z),
                             shape_flag::kDangerous | shape_flag::kVulnerable>>>;

  ShieldHub(ecs::const_handle h) : effect_id{h.id()} {}
  ecs::entity_id effect_id;
  std::uint32_t timer = 0;
  std::uint32_t count = 0;
  vec2 dir{0};
  vec2 target_dir{0};
  std::vector<ecs::entity_id> targets;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    ++timer;
    if (!((sim.tick_count() + +h.id()) % 8u)) {
      static thread_local std::vector<SimInterface::range_info> v;
      v.clear();
      sim.in_range<EnemyStatus>(transform.centre, kShieldDistance, 0, v);
      targets.clear();
      for (const auto& e : v) {
        if (!e.h.get<ShieldHub>()) {
          targets.emplace_back(e.h.id());
        }
      }
    }
    for (auto id : targets) {
      if (auto* status = sim.index().get<EnemyStatus>(id); status) {
        status->shielded_ticks =
            std::max(status->shielded_ticks, std::min(20u, status->shielded_ticks + 2));
      }
    }

    auto dim = sim.dimensions();
    target_dir = transform.centre.x < 96  ? (timer = 0, vec2{1, 0})
        : transform.centre.x > dim.x - 96 ? (timer = 0, vec2{-1, 0})
        : transform.centre.y < 96         ? (timer = 0, vec2{0, 1})
        : transform.centre.y > dim.y - 96 ? (timer = 0, vec2{0, -1})
        : timer >= kTimer ? (timer = 0, from_polar(2 * pi<fixed> * sim.random_fixed(), 1_fx))
                          : target_dir;
    if (sim.is_on_screen(transform.centre)) {
      dir = rc_smooth(dir, target_dir, 31_fx / 32);
    } else {
      dir = target_dir;
    }

    transform.rotate(fixed_c::hundredth * 3);
    transform.move(dir * kSpeed);
    if (auto eh = sim.index().get(effect_id); eh) {
      eh->get<Transform>()->centre = transform.centre;
    }
  }

  void on_destroy(SimInterface& sim) const {
    if (auto eh = sim.index().get(effect_id); eh) {
      eh->get<ShieldEffect>()->destroy = true;
    }
  }

  struct ShieldEffect : ecs::component {
    static constexpr std::uint32_t kFadeInTime = 90;
    static constexpr std::uint32_t kAnimTime = 240;
    static constexpr std::uint32_t kAnimFadeTime = 40;
    using sl0 = geom::set_colour_p<geom::sline(colour::kZero, colour::kZBackgroundEffect1, 4.f), 3>;
    using sl1 = geom::set_colour_p<geom::sline(colour::kZero, colour::kZBackgroundEffect1, 4.f), 4>;
    using sf = geom::set_colour_p<geom::sfill(colour::kZero, colour::kZBackgroundEffect0), 5>;
    using shape =
        standard_transform<geom::ball_eval<geom::constant<geom::bd(kShieldDrawDistance)>, sl0, sf>,
                           geom::ball_eval<geom::set_radius_p<geom::bd(), 2>, sl1>>;

    std::tuple<vec2, fixed, fixed, cvec4, cvec4, cvec4>
    shape_parameters(const Transform& transform) const {
      auto a = static_cast<float>(fade_in) / kFadeInTime;
      auto t = anim % (kAnimTime + kAnimFadeTime);
      auto ta = t < kAnimTime ? 1.f : 1.f - static_cast<float>(t - kAnimTime) / kAnimFadeTime;
      return {transform.centre,
              transform.rotation,
              kShieldDrawDistance * (std::min(t, kAnimTime) / fixed{kAnimTime}),
              colour::alpha(colour::kWhite1, a * colour::kFillAlpha0),
              colour::alpha(colour::kWhite1, a * ta * colour::kFillAlpha0 / 2),
              colour::alpha(colour::kBlack0, a * colour::kBackgroundAlpha0)};
    }

    std::uint32_t anim = 0;
    std::uint32_t fade_in = 0;
    bool destroy = false;

    void update(ecs::handle h, SimInterface&) {
      ++anim;
      destroy&& fade_in && (fade_in -= std::min(fade_in, 3u));
      !destroy&& fade_in < kFadeInTime && ++fade_in;
      if (destroy && !fade_in) {
        h.add(Destroy{});
      }
    }
  };
};
DEBUG_STRUCT_TUPLE(ShieldHub, timer, count, dir);

}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::hub_shape>(h, 112);
  h.add(FollowHub{false, false, fast});
  h.add(Enemy{.threat_value = 6u + 4u * fast});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 20, .bomb_drop_chance = 25});
}

void spawn_big_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::big_hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::big_hub_shape>(h, 112);
  h.add(FollowHub{true, false, fast});
  h.add(Enemy{.threat_value = 12u + 8u * fast});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 35, .bomb_drop_chance = 40});
}

void spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::chaser_hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::chaser_hub_shape>(h, 112);
  h.add(FollowHub{false, true, fast});
  h.add(Enemy{.threat_value = 10u + 6u * fast});
  h.add(DropTable{.shield_drop_chance = 2});
  h.add(DropTable{.bomb_drop_chance = 1});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 30, .bomb_drop_chance = 35});
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default<Shielder>(sim, position);
  add_enemy_health<Shielder>(h, 160);
  h.add(Shielder{sim, power});
  h.add(Enemy{.threat_value = 8u + 2u * power});
  h.add(Physics{.mass = 3_fx});
  h.add(DropTable{.shield_drop_chance = 60});
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default<Tractor>(sim, position);
  add_enemy_health<Tractor>(h, 336);
  h.add(Tractor{power});
  h.add(Enemy{.threat_value = 10u + 4u * power});
  h.add(Physics{.mass = 2_fx});
  h.add(DropTable{.bomb_drop_chance = 100});
}

void spawn_shield_hub(SimInterface& sim, const vec2& position) {
  auto eh = create_ship_default<ShieldHub::ShieldEffect>(sim, position);
  eh.add(ShieldHub::ShieldEffect{});

  auto h = create_ship_default<ShieldHub>(sim, position);
  add_enemy_health<ShieldHub>(h, 224);
  h.add(ShieldHub{eh});
  h.add(Enemy{.threat_value = 10u});
  h.add(Physics{.mass = 3_fx});
  h.add(DropTable{.shield_drop_chance = 80, .bomb_drop_chance = 20});
}

}  // namespace ii::v0
