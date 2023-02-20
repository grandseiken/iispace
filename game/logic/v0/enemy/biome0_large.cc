#include "game/common/colour.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"

namespace ii::v0 {
namespace {
using namespace geom2;

struct FollowHub : ecs::component {
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 200;
  static constexpr fixed kSpeed = 15_fx / 16_fx;
  static constexpr fixed kBoundingWidth = 18;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;
  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c = colour::kSolarizedDarkBlue;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  static void construct_shape(node& root, std::uint32_t type) {
    static constexpr auto outline = sline(colour::kOutline, colour::kZOutline, 2.f);

    auto& n = root.add(translate{key{'v'}});
    auto& centre = n.add(rotate{pi<fixed> / 4});
    auto& r = n.add(rotate{key{'r'}});

    centre.add(ngon{.dimensions = nd(20, 4), .line = outline});
    centre.add(ngon{.dimensions = nd(18, 4),
                    .style = ngon_style::kPolygram,
                    .line = sline(c, z),
                    .fill = sfill(cf, z)});
    centre.add(ngon_collider{.dimensions = nd(18, 4), .flags = kFlags});

    auto& spoke = root.create(compound{});
    r.add(translate{vec2{18, 0}}).add(spoke);
    r.add(translate{vec2{-18, 0}}).add(spoke);
    r.add(translate{vec2{0, 18}}).add(spoke);
    r.add(translate{vec2{0, -18}}).add(spoke);
    auto& spoke_r = spoke.add(rotate{pi<fixed> / 4});

    spoke_r.add(ngon{.dimensions = nd(12, 4), .line = outline});
    spoke_r.add(ngon{.dimensions = nd(10, 4),
                     .style = type == 2 ? ngon_style::kPolygram : ngon_style::kPolygon,
                     .line = sline(c, z),
                     .fill = sfill(cf, z)});
    if (type == 1) {
      spoke_r.add(ngon{.dimensions = nd(6, 4), .line = sline(c, z, 2.f)});
    }
  }

  static void construct_hub_shape(node& root) { construct_shape(root, 0); }
  static void construct_big_hub_shape(node& root) { construct_shape(root, 1); }
  static void construct_chaser_hub_shape(node& root) { construct_shape(root, 2); }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
  }

  FollowHub(bool big, bool chaser, bool fast) : big{big}, chaser{chaser}, fast{fast} {}
  std::uint32_t timer = 0;
  std::uint32_t count = 0;
  vec2 dir{0};
  bool big = false;
  bool chaser = false;
  bool fast = false;

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

  void on_destroy(const Transform& transform, const EnemyStatus& status, SimInterface& sim,
                  EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb || status.destroy_timer) {
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
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 40;
  static constexpr fixed kSpeed = 1_fx;
  static constexpr fixed kBoundingWidth = 32;
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kWeakShield;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c0 = colour::linear_mix(colour::kSolarizedDarkCyan, colour::kNewGreen0);
  static constexpr auto c1 = colour::kWhite1;
  static constexpr auto cf = colour::alpha(c0, colour::kFillAlpha0);

  static void construct_shape(node& root) {
    static constexpr auto outline = sline(colour::kOutline, colour::kZOutline, 2.f);

    auto& n = root.add(translate{key{'v'}});
    auto& centre = n.add(rotate{key{'r'}});
    auto& shield = n.add(rotate{key{'R'}});

    centre.add(ngon{.dimensions = nd(22, 12), .line = outline});
    centre.add(ngon{.dimensions = nd2(29, 6, 12),
                    .style = ngon_style::kPolystar,
                    .line = sline(colour::kOutline, colour::kZOutline, 5.f)});
    centre.add(ngon{
        .dimensions = nd2(27 + 1_fx / 2, 6, 12),
        .style = ngon_style::kPolystar,
        .line = sline(c0, z),
    });
    centre.add(ngon{.dimensions = nd(6, 12), .line = sline(c0, z)});
    centre.add(ngon{.dimensions = nd(20, 12), .line = sline(c0, z), .fill = sfill(cf, z)});
    centre.add(ngon_collider{.dimensions = nd(20, 12),
                             .flags = shape_flag::kDangerous | shape_flag::kVulnerable});

    shield.add(line{.a = vec2{32, 0}, .b = vec2{18, 0}, .style = sline(c1, z)});
    shield.add(rotate{pi<fixed> / 4})
        .add(line{.a = vec2{-32, 0}, .b = vec2{-18, 0}, .style = sline(c1, z)});

    shield.add(ngon{.dimensions = nd2(27 + 1_fx / 2, 22, 16, 10), .fill = sfill(cf, z)});
    shield.add(ngon{.dimensions = nd2(27 + 1_fx / 2, 22, 16, 10),
                    .fill = sfill(colour::alpha(c1, colour::kFillAlpha0), z)});
    shield.add(ngon{.dimensions = nd(29, 16, 10), .line = outline});

    shield.add(ngon{.dimensions = nd(30, 16, 10), .line = sline(c1, z, 1.25f)});
    shield.add(ngon{.dimensions = nd(31, 16, 10), .line = outline});
    shield.add(ngon{.dimensions = nd(32, 16, 10), .line = sline(c1, z, 1.25f)});
    shield.add(ngon{.dimensions = nd(34, 16, 10), .line = outline});

    shield.add(ngon_collider{.dimensions = nd(32, 16, 10), .flags = shape_flag::kWeakShield});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'R'}, shield_angle + pi<fixed> / 2 - pi<fixed> / 8);
  }

  Shielder(SimInterface& sim, bool power)
  : timer{sim.random(random_source::kGameSequence).uint(kTimer)}
  , power{power}
  , spreader{.max_distance = 128_fx, .max_n = 4u} {}
  std::uint32_t timer = 0;
  vec2 velocity{0, 0};
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;
  bool power = false;
  bool on_screen = false;
  fixed shield_angle = 0;
  Spreader spreader;

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

    spreader.spread_closest<Shielder>(h, transform, sim, 64 * kSpeed,
                                      Spreader::linear_coefficient(4_fx));
  }
};
DEBUG_STRUCT_TUPLE(Shielder, timer, velocity, target, next_target, spreader, power, on_screen,
                   shield_angle);

// TODO: add hard variant that shoots... bounces?
struct Tractor : ecs::component {
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 60;
  static constexpr fixed kSpeed = 6 * (15_fx / 160);
  static constexpr fixed kPullSpeed = 2 + 3_fx / 8;
  static constexpr fixed kBoundingWidth = 45;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c = colour::kSolarizedDarkMagenta;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  static void construct_shape(node& root) {
    static constexpr auto outline = sline(colour::kOutline, colour::kZOutline, 2.f);

    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    auto& orb = root.create(compound{});
    auto& star = root.create(compound{});

    n.add(translate_rotate{vec2{26, 0}, key{'S'}}).add(orb);
    n.add(translate_rotate{vec2{-26, 0}, key{'s'}}).add(orb);
    n.add(line{.a = vec2{-26, 0},
               .b = vec2{26, 0},
               .style = sline(colour::kOutline, colour::kZOutline, 5.f)});
    n.add(line{.a = vec2{-26, 0}, .b = vec2{26, 0}, .style = sline(c, z)});

    auto& star_if = n.add(enable{key{'P'}});
    star_if.add(translate_rotate{vec2{26, 0}, key{'T'}}).add(star);
    star_if.add(translate_rotate{vec2{-26, 0}, key{'t'}}).add(star);

    orb.add(ngon{.dimensions = nd(18, 6), .line = outline});
    orb.add(ngon{.dimensions = nd(16, 6),
                 .style = ngon_style::kPolygram,
                 .line = sline(c, z),
                 .fill = sfill(cf, z)});
    orb.add(ngon_collider{.dimensions = nd(16, 6), .flags = kFlags});

    star.add(ngon{.dimensions = nd(18, 6),
                  .style = ngon_style::kPolystar,
                  .line = sline(colour::kOutline, colour::kZOutline, 5.f)});
    star.add(ngon{.dimensions = nd(18, 6), .style = ngon_style::kPolystar, .line = sline(c, z)});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'s'}, -5 * spoke_r)
        .add(key{'S'}, 5 * spoke_r)
        .add(key{'t'}, -8 * spoke_r)
        .add(key{'T'}, 8 * spoke_r)
        .add(key{'P'}, power);
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
          auto index = render::tag_t{'p' + pc.player_number};
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

// TODO: is the shield darken area rendered over FX?
// TODO: redesign look.
// TODO: redesign shield status effect visuals.
struct ShieldHub : ecs::component {
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 280;
  static constexpr fixed kSpeed = 3_fx / 4_fx;
  static constexpr fixed kShieldDistance = 250;
  static constexpr fixed kShieldDrawDistance = kShieldDistance + 15;
  static constexpr fixed kBoundingWidth = 28;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static constexpr auto z = colour::kZEnemyLarge;
  static constexpr auto c0 = colour::linear_mix(colour::kSolarizedDarkCyan, colour::kNewGreen0);
  static constexpr auto c1 = colour::kWhite1;
  static constexpr auto cf0 = colour::alpha(c0, colour::kFillAlpha0);
  static constexpr auto cf1 = colour::alpha(c1, colour::kFillAlpha1);

  static void construct_shape(node& root) {
    static constexpr auto outline = sline(colour::kOutline, colour::kZOutline, 2.f);

    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    auto& t = root.add(translate{key{'v'}});
    auto& inner = root.create(compound{});
    auto& inner_box = root.create(compound{});

    n.add(ball{.dimensions = bd(kBoundingWidth + 2), .line = outline});
    n.add(ball{.dimensions = bd(kBoundingWidth), .line = sline(c0, z), .fill = sfill(cf0, z)});
    n.add(ball_collider{.dimensions = bd(kBoundingWidth), .flags = kFlags});

    t.add(ball{.dimensions = bd(kBoundingWidth / 3), .line = sline(c0, z)});
    t.add(rotate{key{'s'}}).add(inner);
    t.add(rotate{key{'S'}}).add(inner);

    inner.add(translate{vec2{-18, 0}}).add(inner_box);
    inner.add(translate{vec2{18, 0}}).add(inner_box);
    inner.add(box{.dimensions = vec2{11, 4}, .line = sline(c1, z)});
    inner_box.add(box{.dimensions = vec2{10, 10}, .line = outline});
    inner_box.add(box{.dimensions = vec2{8, 8}, .line = sline(c1, z), .fill = sfill(cf1, z)});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'s'}, -transform.rotation / 2)
        .add(key{'S'}, transform.rotation / 2);
  }

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
      targets.clear();
      for (const auto& c :
           sim.collide(check_ball(shape_flag::kVulnerable | shape_flag::kWeakVulnerable,
                                  transform.centre, kShieldDistance))) {
        if (!c.h.get<ShieldHub>()) {
          targets.emplace_back(c.h.id());
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
    static constexpr auto kBoundingWidth = 0_fx;
    static constexpr auto kFlags = shape_flag::kNone;

    static void construct_shape(node& root) {
      auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
      n.add(ball{.dimensions = bd(kShieldDrawDistance),
                 .line = {.colour0 = key{'0'}, .z = colour::kZBackgroundEffect1, .width = 4.f},
                 .fill = {.colour0 = key{'2'}, .z = colour::kZBackgroundEffect1}});
      n.add(ball{.dimensions = {.radius = key{'R'}},
                 .line = {.colour0 = key{'1'}, .z = colour::kZBackgroundEffect1, .width = 4.f}});
    }

    void set_parameters(const Transform& transform, parameter_set& parameters) const {
      auto a = static_cast<float>(fade_in) / kFadeInTime;
      auto t = anim % (kAnimTime + kAnimFadeTime);
      auto ta = t < kAnimTime ? 1.f : 1.f - static_cast<float>(t - kAnimTime) / kAnimFadeTime;

      parameters.add(key{'v'}, transform.centre)
          .add(key{'r'}, transform.rotation)
          .add(key{'R'}, kShieldDrawDistance * (std::min(t, kAnimTime) / fixed{kAnimTime}))
          .add(key{'0'}, colour::alpha(colour::kWhite1, a * colour::kFillAlpha0))
          .add(key{'1'}, colour::alpha(colour::kWhite1, a * ta * colour::kFillAlpha0 / 2))
          .add(key{'2'}, colour::alpha(colour::kBlack0, a * colour::kBackgroundAlpha0));
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

ecs::handle spawn_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  using shape =
      shape_definition<&FollowHub::construct_hub_shape, ecs::call<&FollowHub::set_parameters>,
                       FollowHub::kBoundingWidth, FollowHub::kFlags>;

  auto h = create_ship_default2<FollowHub, shape>(sim, position);
  add_enemy_health2<FollowHub, shape>(h, 112);
  h.add(FollowHub{false, false, fast});
  h.add(Enemy{.threat_value = 6u + 4u * fast});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 20, .bomb_drop_chance = 25});
  return h;
}

ecs::handle spawn_big_follow_hub(SimInterface& sim, const vec2& position, bool fast) {
  using shape =
      shape_definition<&FollowHub::construct_big_hub_shape, ecs::call<&FollowHub::set_parameters>,
                       FollowHub::kBoundingWidth, FollowHub::kFlags>;

  auto h = create_ship_default2<FollowHub, shape>(sim, position);
  add_enemy_health2<FollowHub, shape>(h, 112);
  h.add(FollowHub{true, false, fast});
  h.add(Enemy{.threat_value = 12u + 8u * fast});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 35, .bomb_drop_chance = 40});
  return h;
}

ecs::handle spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast) {
  using shape = shape_definition<&FollowHub::construct_chaser_hub_shape,
                                 ecs::call<&FollowHub::set_parameters>, FollowHub::kBoundingWidth,
                                 FollowHub::kFlags>;

  auto h = create_ship_default2<FollowHub, shape>(sim, position);
  add_enemy_health2<FollowHub, shape>(h, 112);
  h.add(FollowHub{false, true, fast});
  h.add(Enemy{.threat_value = 10u + 6u * fast});
  h.add(DropTable{.shield_drop_chance = 2});
  h.add(DropTable{.bomb_drop_chance = 1});
  h.add(Physics{.mass = 1_fx + 3_fx / 4});
  h.add(DropTable{.shield_drop_chance = 30, .bomb_drop_chance = 35});
  return h;
}

ecs::handle spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default2<Shielder>(sim, position);
  add_enemy_health2<Shielder>(h, 160);
  h.add(Shielder{sim, power});
  h.add(Enemy{.threat_value = 8u + 2u * power});
  h.add(Physics{.mass = 3_fx});
  h.add(DropTable{.shield_drop_chance = 60});
  return h;
}

ecs::handle spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship_default2<Tractor>(sim, position);
  add_enemy_health2<Tractor>(h, 336);
  h.add(Tractor{power});
  h.add(Enemy{.threat_value = 10u + 4u * power});
  h.add(Physics{.mass = 2_fx});
  h.add(DropTable{.bomb_drop_chance = 100});
  return h;
}

ecs::handle spawn_shield_hub(SimInterface& sim, const vec2& position) {
  auto eh = create_ship_default2<ShieldHub::ShieldEffect>(sim, position);
  eh.add(ShieldHub::ShieldEffect{});

  auto h = create_ship_default2<ShieldHub>(sim, position);
  add_enemy_health2<ShieldHub>(h, 280);
  h.add(ShieldHub{eh});
  h.add(Enemy{.threat_value = 10u});
  h.add(Physics{.mass = 3_fx});
  h.add(DropTable{.shield_drop_chance = 80, .bomb_drop_chance = 20});
  return h;
}

}  // namespace ii::v0
