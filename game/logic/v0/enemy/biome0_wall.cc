#include "game/common/colour.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {
using namespace geom2;

struct Square : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;
  static constexpr fixed kSpeed = 1_fx + 3_fx / 4_fx;
  static constexpr fixed kBoundingWidth = 14;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;
  static constexpr auto z = colour::kZEnemyWall;

  // TODO: box outline shadows have odd overlaps? Not really sure why since it
  // should line up exactly. Happens even when rotatated...
  // In screenshots: outline is 3px, inner line is 2px; but shadows overlap by 2px
  // (so really outline is 4px and inner is 2px)?
  // Shadow overlap could be fixed by moving outlines/shadows into render::shapes (and only
  // putting out 1 shadow), but why is it overlapping to begin with?
  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(box_collider{.dimensions = vec2{12, 12}, .flags = kFlags});
    n.add(box{
        .dimensions = vec2{14, 14},
        .line = {.colour0 = colour::kOutline, .z = colour::kZOutline, .width = 2.f},
    });
    n.add(box{
        .dimensions = vec2{12, 12},
        .line = {.colour0 = key{'c'}, .z = z, .width = 1.5f},
        .fill = {.colour0 = key{'f'}, .z = z},
    });
  }

  void set_parameters(const Transform& transform, const Health& health,
                      parameter_set& parameters) const {
    auto c = colour::kNewGreen0;
    if (health.hp && invisible_flash) {
      c = colour::alpha(c, (5.f + 3.f * std::cos(invisible_flash / pi<float>)) / 8.f);
    }
    auto cf = colour::alpha(c, colour::kFillAlpha0);

    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, c)
        .add(key{'f'}, cf);
  }

  Square(SimInterface& sim, const vec2& dir) : dir{dir}, timer{sim.random(80) + 40} {}
  vec2 dir{0};
  std::uint32_t timer = 0;
  std::uint32_t invisible_flash = 0;

  void update(ecs::handle h, Transform& transform, Health& health, SimInterface& sim) {
    bool vulnerable = sim.global_entity().get<GlobalData>()->walls_vulnerable;
    if (sim.is_on_screen(transform.centre) && vulnerable) {
      if (timer) {
        --timer;
      }
    } else {
      timer = sim.random(h).uint(80) + 40;
    }

    if (!timer) {
      health.damage(h, sim, 4, damage_type::kNormal, h.id());
    }

    const vec2& v = transform.centre;
    auto dim = sim.dimensions();
    if (v.x < 0 && dir.x <= 0) {
      dir.x = -dir.x;
      if (dir.x <= 0) {
        dir.x = 1;
      }
    }
    if (v.y < 0 && dir.y <= 0) {
      dir.y = -dir.y;
      if (dir.y <= 0) {
        dir.y = 1;
      }
    }
    if (v.x > dim.x && dir.x >= 0) {
      dir.x = -dir.x;
      if (dir.x >= 0) {
        dir.x = -1;
      }
    }
    if (v.y > dim.y && dir.y >= 0) {
      dir.y = -dir.y;
      if (dir.y >= 0) {
        dir.y = -1;
      }
    }
    dir = normalise(dir);
    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
    if (vulnerable) {
      ++invisible_flash;
    } else {
      invisible_flash = 0;
    }
  }
};
DEBUG_STRUCT_TUPLE(Square, dir, timer, invisible_flash);

// TODO: should wall go faster offscreen?
struct Wall : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;

  static constexpr std::uint32_t kTimer = 100;
  static constexpr fixed kSpeed = 1;
  static constexpr fixed kBoundingWidth = 60;
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kWeakVulnerable;

  static constexpr auto z = colour::kZEnemyWall;
  static constexpr auto c = colour::kNewGreen0;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(box_collider{.dimensions = vec2{12, 48}, .flags = key{'C'}});
    n.add(box{
        .dimensions = vec2{14, 50},
        .line = {.colour0 = colour::kOutline, .z = colour::kZOutline, .width = 2.f},
    });
    n.add(box{
        .dimensions = vec2{12, 48},
        .line = {.colour0 = c, .z = z, .width = 1.75f},
        .fill = {.colour0 = cf, .z = z},
    });
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'C'},
             shape_flag::kDangerous |
                 (weak ? shape_flag::kWeakVulnerable : shape_flag::kVulnerable));
  }

  Wall(const vec2& dir, bool anti) : dir{dir}, anti{anti} {}
  vec2 dir{0};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool anti = false;
  bool weak = false;

  void update(ecs::handle h, Transform& transform, Health& health, SimInterface& sim) {
    if (sim.global_entity().get<GlobalData>()->walls_vulnerable && timer && !(timer % 8)) {
      weak = true;
      if (health.hp > 2 && sim.is_on_screen(transform.centre)) {
        sim.emit(resolve_key::predicted()).play(sound::kEnemySpawn, 1.f, 0.f);
      }
      health.damage(h, sim, std::max(2u, health.hp) - 2, damage_type::kNormal, h.id());
    }

    if (is_rotating) {
      auto d = ::rotate(dir, (anti ? -1 : 1) * (kTimer - timer) * pi<fixed> / (4 * kTimer));

      transform.set_rotation(angle(d));
      if (!--timer) {
        is_rotating = false;
        dir = ::rotate(dir, anti ? -pi<fixed> / 4 : pi<fixed> / 4);
      }
      return;
    }
    if (sim.is_on_screen(transform.centre)) {
      if (++timer > kTimer * 6) {
        timer = kTimer;
        is_rotating = true;
      }
    }

    const auto& v = transform.centre;
    auto dim = sim.dimensions();
    if ((v.x < 0 && dir.x < -fixed_c::hundredth) || (v.y < 0 && dir.y < -fixed_c::hundredth) ||
        (v.x > dim.x && dir.x > fixed_c::hundredth) ||
        (v.y > dim.y && dir.y > fixed_c::hundredth)) {
      dir = -normalise(dir);
    }

    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
  }

  void on_destroy(const Transform& transform, const EnemyStatus& status, SimInterface& sim,
                  EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb || status.destroy_timer) {
      return;
    }

    auto p = from_polar(transform.rotation, 1_fx);
    auto d = ::rotate(p, pi<fixed> / 2);
    auto v = transform.centre + d * 12 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, p, /* drop */ false);
    }
    v = transform.centre - d * 12 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, p, /* drop */ false);
    }
  }
};
DEBUG_STRUCT_TUPLE(Wall, dir, timer, is_rotating, anti);
}  // namespace

ecs::handle spawn_square(SimInterface& sim, const vec2& position, const vec2& dir, bool drop) {
  auto h = create_ship_default<Square>(sim, position);
  add_enemy_health<Square>(h, 32);
  h.add(Square{sim, dir});
  h.add(Enemy{.threat_value = 2});
  h.add(WallTag{});
  h.add(Physics{.mass = 1_fx + 1_fx / 2});
  if (drop) {
    h.add(DropTable{.shield_drop_chance = 2, .bomb_drop_chance = 2});
  }
  return h;
}

ecs::handle spawn_wall(SimInterface& sim, const vec2& position, const vec2& dir, bool anti) {
  auto h = create_ship_default<Wall>(sim, position);
  add_enemy_health<Wall>(h, 80);
  h.add(Wall{dir, anti});
  h.add(Enemy{.threat_value = 4});
  h.add(WallTag{});
  h.add(Physics{.mass = 2_fx});
  h.add(DropTable{.shield_drop_chance = 3, .bomb_drop_chance = 4});
  return h;
}

}  // namespace ii::v0
