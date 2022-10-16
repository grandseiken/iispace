#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/geometry/shapes/line.h"
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

  static constexpr std::uint32_t kTimer = 100;
  static constexpr fixed kSpeed = 2_fx;

  static constexpr auto c0 = colour_hue360(150, .2f);
  static constexpr auto c1 = colour_hue360(160, .5f, .6f);
  static constexpr auto c2 = glm::vec4{0.f, 0.f, .75f, 1.f};

  using centre_shape = geom::compound<
      geom::polystar<26, 12, c0>,
      geom::polygon<6, 12, c0>,
      geom::polygon<20, 12, c1, shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using shield_shape = geom::rotate_p<2,
      geom::line<32, 0, 18, 0, c2>,
      geom::rotate<fixed_c::pi / 4, geom::line<-32, 0, -18, 0, c2>>,
      geom::polyarc<26, 16, 10, c2>,
      geom::polyarc<32, 16, 10, c2, shape_flag::kWeakShield>>;
  using shape = geom::translate_p<0, geom::rotate_p<1, centre_shape>, shield_shape>;

  std::tuple<vec2, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, shield_angle + fixed_c::pi / 2 - fixed_c::pi / 8};
  }

  Shielder(bool power) : power{power} {}
  std::uint32_t timer = 0;
  vec2 dir{0, 1};
  bool is_rotating = false;
  bool rotate_anti = false;
  bool power = false;
  fixed shield_angle = 0;

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    fixed s = power ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
    transform.rotate(s);

    static constexpr fixed kMaxRotationSpeed = 1_fx / 16_fx;
    auto smooth_rotate = [&](fixed& x, fixed target) {
      auto d = angle_diff(x, target);
      if (abs(d) < kMaxRotationSpeed) {
        x = target;
      } else {
        x += std::clamp(d, -kMaxRotationSpeed, kMaxRotationSpeed);
      }
    };

    bool on_screen = false;
    auto dim = sim.dimensions();
    dir = transform.centre.x < 0     ? vec2{1, 0}
        : transform.centre.x > dim.x ? vec2{-1, 0}
        : transform.centre.y < 0     ? vec2{0, 1}
        : transform.centre.y > dim.y ? vec2{0, -1}
                                     : (on_screen = true, dir);

    if (!on_screen && is_rotating) {
      timer = 0;
      is_rotating = false;
    }

    fixed speed = kSpeed + (power ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (20 - health.hp);
    if (is_rotating) {
      auto d = rotate(dir, (rotate_anti ? 1 : -1) * (kTimer - timer) * fixed_c::pi / (2 * kTimer));
      if (!--timer) {
        is_rotating = false;
        dir = rotate(dir, (rotate_anti ? 1 : -1) * fixed_c::pi / 2);
      }
      transform.move(d * speed);
      smooth_rotate(shield_angle, angle(d));
    } else {
      ++timer;
      if (timer > kTimer * 2) {
        timer = kTimer;
        is_rotating = true;
        rotate_anti = sim.random_bool();
      }
      if (sim.is_on_screen(transform.centre) && power && timer % kTimer == kTimer / 2) {
        // spawn_boss_shot(sim, transform.centre, 3 *
        // sim.nearest_player_direction(transform.centre),
        //                 colour_hue360(160, .5f, .6f));
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }
      transform.move(dir * speed);
      smooth_rotate(shield_angle, angle(dir));
    }
    dir = normalise(dir);
  }
};
DEBUG_STRUCT_TUPLE(Shielder, timer, dir, is_rotating, rotate_anti, power, shield_angle);

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
  h.add(Shielder{power});
  h.add(Enemy{.threat_value = 8u + 2u * power});
}

}  // namespace ii::v0
