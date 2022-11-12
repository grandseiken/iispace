#include "game/common/colour.h"
#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {

struct Square : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 14;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;
  static constexpr fixed kSpeed = 1_fx + 3_fx / 4_fx;

  // TODO: box outline shadows have odd overlaps? Not really sure why since it
  // should line up exactly. Happens even when rotatated...
  static constexpr auto z = colour::kZEnemyWall;
  using shape = standard_transform<
      geom::box_collider<vec2{12, 12}, shape_flag::kDangerous | shape_flag::kVulnerable>,
      geom::box<vec2{14, 14}, geom::sline(colour::kOutline, colour::kZOutline, 2.f)>,
      geom::box_colour_p2<vec2{12, 12}, 2, 3, geom::sline(colour::kZero, z, 1.5f),
                          geom::sfill(colour::kZero, z)>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4>
  shape_parameters(const Transform& transform, const Health& health) const {
    auto c = colour::kNewGreen0;
    if (health.hp && invisible_flash) {
      c = colour::alpha(c, .5f);
    }
    auto cf = colour::alpha(c, .25f);
    return {transform.centre, transform.rotation, c, cf};
  }

  vec2 dir{0};
  std::uint32_t timer = 0;
  bool invisible_flash = false;

  Square(SimInterface& sim, const vec2& dir) : dir{dir}, timer{sim.random(80) + 40} {}

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
      health.damage(h, sim, 4, damage_type::kNormal, h.id());
    }

    const vec2& v = transform.centre;
    auto dim = sim.dimensions();
    if (v.x < 0 && dir.x <= 0) {
      dir.x = -dir.x;
      if (dir.x <= 0) {
        dir.x = 1;
      }
      render.clear_trails = true;
    }
    if (v.y < 0 && dir.y <= 0) {
      dir.y = -dir.y;
      if (dir.y <= 0) {
        dir.y = 1;
      }
      render.clear_trails = true;
    }
    if (v.x > dim.x && dir.x >= 0) {
      dir.x = -dir.x;
      if (dir.x >= 0) {
        dir.x = -1;
      }
      render.clear_trails = true;
    }
    if (v.y > dim.y && dir.y >= 0) {
      dir.y = -dir.y;
      if (dir.y >= 0) {
        dir.y = -1;
      }
      render.clear_trails = true;
    }
    dir = normalise(dir);
    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
    invisible_flash = no_enemies && (timer % 4 == 1 || timer % 4 == 2);
  }
};
DEBUG_STRUCT_TUPLE(Square, dir, timer, invisible_flash);

struct Wall : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 60;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLow;

  static constexpr std::uint32_t kTimer = 100;
  static constexpr fixed kSpeed = 1;

  static constexpr auto z = colour::kZEnemyWall;
  static constexpr auto c = colour::kNewGreen0;
  static constexpr auto cf = colour::alpha(c, .25f);
  using shape = standard_transform<
      geom::box<vec2{14, 50}, geom::sline(colour::kOutline, colour::kZOutline, 2.f)>,
      geom::conditional_p<
          2,
          geom::box_with_collider<vec2{12, 48}, geom::sline(c, z, 1.75f), geom::sfill(cf, z),
                                  shape_flag::kDangerous | shape_flag::kWeakVulnerable>,
          geom::box_with_collider<vec2{12, 48}, geom::sline(c, z, 1.75f), geom::sfill(cf, z),
                                  shape_flag::kDangerous | shape_flag::kVulnerable>>>;

  Wall(const vec2& dir, bool anti) : dir{dir}, anti{anti} {}
  vec2 dir{0};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool anti = false;
  bool weak = false;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, weak};
  }

  void
  update(ecs::handle h, Transform& transform, Render& render, Health& health, SimInterface& sim) {
    if (!sim.global_entity().get<GlobalData>()->non_wall_enemy_count && timer && timer % 8 < 2) {
      weak = true;
      if (health.hp > 2) {
        sim.emit(resolve_key::predicted()).play(sound::kEnemySpawn, 1.f, 0.f);
      }
      health.damage(h, sim, std::max(2u, health.hp) - 2, damage_type::kNormal, h.id());
    }

    if (is_rotating) {
      auto d = rotate(dir, (anti ? -1 : 1) * (kTimer - timer) * fixed_c::pi / (4 * kTimer));

      transform.set_rotation(angle(d));
      if (!--timer) {
        is_rotating = false;
        dir = rotate(dir, anti ? -fixed_c::pi / 4 : fixed_c::pi / 4);
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
      render.clear_trails = true;
    }

    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }

    auto p = from_polar(transform.rotation, 1_fx);
    auto d = rotate(p, fixed_c::pi / 2);
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

void spawn_square(SimInterface& sim, const vec2& position, const vec2& dir, bool drop) {
  auto h = create_ship_default<Square>(sim, position);
  add_enemy_health<Square>(h, 32);
  h.add(Square{sim, dir});
  h.add(Enemy{.threat_value = 2});
  h.add(WallTag{});
  if (drop) {
    h.add(DropTable{.shield_drop_chance = 2, .bomb_drop_chance = 2});
  }
}

void spawn_wall(SimInterface& sim, const vec2& position, const vec2& dir, bool anti) {
  auto h = create_ship_default<Wall>(sim, position);
  add_enemy_health<Wall>(h, 80);
  h.add(Wall{dir, anti});
  h.add(Enemy{.threat_value = 4});
  h.add(WallTag{});
  h.add(DropTable{.shield_drop_chance = 3, .bomb_drop_chance = 4});
}

}  // namespace ii::v0
