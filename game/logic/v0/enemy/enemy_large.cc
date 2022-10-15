#include "game/logic/geometry/shapes/ngon.h"
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
  h.add(Enemy{.threat_value = 10u + 6u * fast});
}

void spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast) {
  auto h = create_ship_default<FollowHub, FollowHub::chaser_hub_shape>(sim, position);
  add_enemy_health<FollowHub, FollowHub::chaser_hub_shape>(h, 14);
  h.add(FollowHub{false, true, fast});
  h.add(Enemy{.threat_value = 10u + 6u * fast});
}

}  // namespace ii::v0
