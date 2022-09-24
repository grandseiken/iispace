#include "game/logic/geometry/shapes/box.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>

namespace ii::legacy {
namespace {
struct Square : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 15;
  static constexpr float kZIndex = -8.f;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;
  static constexpr fixed kSpeed = 2 + 1_fx / 4;
  using shape = standard_transform<
      geom::box_colour_p<10, 10, 2, shape_flag::kDangerous | shape_flag::kVulnerable>>;

  std::tuple<vec2, fixed, glm::vec4>
  shape_parameters(const Transform& transform, const Health& health) const {
    return {transform.centre, transform.rotation,
            health.hp && invisible_flash ? colour_hue(0.f, .2f, 0.f) : colour_hue360(120, .6f)};
  }

  vec2 dir{0};
  std::uint32_t timer = 0;
  bool invisible_flash = false;

  Square(SimInterface& sim, fixed dir_angle)
  : dir{from_polar(dir_angle, 1_fx)}, timer{sim.random(80) + 40} {}

  void
  update(ecs::handle h, Transform& transform, Render& render, Health& health, SimInterface& sim) {
    bool no_enemies = !sim.global_entity().get<GlobalData>()->non_wall_enemy_count;
    if (sim.is_on_screen(transform.centre) && no_enemies) {
      if (timer) {
        --timer;
      }
    } else {
      timer = sim.random(80) + 40;
    }

    if (!timer) {
      health.damage(h, sim, 4, damage_type::kNone, h.id());
    }

    const vec2& v = transform.centre;
    auto dim = sim.dimensions();
    if (v.x < 0 && dir.x <= 0) {
      dir.x = -dir.x;
      if (dir.x <= 0) {
        dir.x = 1;
      }
      render.trails.clear();
    }
    if (v.y < 0 && dir.y <= 0) {
      dir.y = -dir.y;
      if (dir.y <= 0) {
        dir.y = 1;
      }
      render.trails.clear();
    }
    if (v.x > dim.x && dir.x >= 0) {
      dir.x = -dir.x;
      if (dir.x >= 0) {
        dir.x = -1;
      }
      render.trails.clear();
    }
    if (v.y > dim.y && dir.y >= 0) {
      dir.y = -dir.y;
      if (dir.y >= 0) {
        dir.y = -1;
      }
      render.trails.clear();
    }
    dir = normalise(dir);
    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
    invisible_flash = no_enemies && (timer % 4 == 1 || timer % 4 == 2);
  }
};
DEBUG_STRUCT_TUPLE(Square, dir, timer, invisible_flash);

struct Wall : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 50;
  static constexpr float kZIndex = -8.f;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;

  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 1 + 1_fx / 4;
  using shape = standard_transform<geom::box<10, 40, colour_hue360(120, .5f, .6f),
                                             shape_flag::kDangerous | shape_flag::kVulnerable>>;

  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool rdir = false;

  Wall(bool rdir) : rdir{rdir} {}

  void
  update(ecs::handle h, Transform& transform, Render& render, Health& health, SimInterface& sim) {
    if (!sim.global_entity().get<GlobalData>()->non_wall_enemy_count && timer % 8 < 2) {
      if (health.hp > 2) {
        sim.emit(resolve_key::predicted()).play(sound::kEnemySpawn, 1.f, 0.f);
      }
      health.damage(h, sim, std::max(2u, health.hp) - 2, damage_type::kNone, h.id());
    }

    if (is_rotating) {
      auto d = sim.rotate_compatibility(
          dir, (rdir ? -1 : 1) * (kTimer - timer) * fixed_c::pi / (4 * kTimer));

      transform.set_rotation(angle(d));
      if (!--timer) {
        is_rotating = false;
        dir = sim.rotate_compatibility(dir, rdir ? -fixed_c::pi / 4 : fixed_c::pi / 4);
      }
      return;
    }
    if (++timer > kTimer * 6) {
      if (sim.is_on_screen(transform.centre)) {
        timer = kTimer;
        is_rotating = true;
      } else {
        timer = 0;
      }
    }

    const auto& v = transform.centre;
    auto dim = sim.dimensions();
    if ((v.x < 0 && dir.x < -fixed_c::hundredth) || (v.y < 0 && dir.y < -fixed_c::hundredth) ||
        (v.x > dim.x && dir.x > fixed_c::hundredth) ||
        (v.y > dim.y && dir.y > fixed_c::hundredth)) {
      dir = -normalise(dir);
      render.trails.clear();
    }

    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    auto d = sim.rotate_compatibility(dir, fixed_c::pi / 2);
    auto v = transform.centre + d * 10 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, transform.rotation);
    }
    v = transform.centre - d * 10 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, transform.rotation);
    }
  }
};
DEBUG_STRUCT_TUPLE(Wall, dir, timer, is_rotating, rdir);
}  // namespace

void spawn_square(SimInterface& sim, const vec2& position, fixed dir_angle) {
  auto h = create_ship<Square>(sim, position);
  add_enemy_health<Square>(h, 4);
  h.add(Square{sim, dir_angle});
  h.add(Enemy{.threat_value = 2, .score_reward = 25});
  h.add(WallTag{});
}

void spawn_wall(SimInterface& sim, const vec2& position, bool rdir) {
  auto h = create_ship<Wall>(sim, position);
  add_enemy_health<Wall>(h, 10);
  h.add(Wall{rdir});
  h.add(Enemy{.threat_value = 4, .score_reward = 20});
  h.add(WallTag{});
}

}  // namespace ii::legacy
