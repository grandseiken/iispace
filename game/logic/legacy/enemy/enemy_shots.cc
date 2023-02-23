#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"
#include <algorithm>

namespace ii::legacy {
namespace {
using namespace geom;

struct BossShot : ecs::component {
  static constexpr float kZIndex = 16.f;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr fixed kBoundingWidth = 12;
  static constexpr auto kFlags = shape_flag::kDangerous;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(10, 8), .line = {.colour0 = key{'c'}}});
    n.add(ngon{
        .dimensions = nd(16, 8), .style = ngon_style::kPolystar, .line = {.colour0 = key{'c'}}});
    n.add(ball_collider{.dimensions = bd(12), .flags = kFlags});
    n.add(/* dummy */ ball{.line = {.colour0 = key{'c'}}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, colour);
  }

  BossShot(const vec2& velocity, const cvec4& colour, fixed rotate_speed)
  : velocity{velocity}, colour{colour}, rotate_speed{rotate_speed} {}
  std::uint32_t timer = 0;
  vec2 velocity{0};
  cvec4 colour{0.f};
  fixed rotate_speed = 0;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    transform.move(velocity);
    vec2 p = transform.centre;
    auto d = sim.dimensions();
    if ((p.x < -10 && velocity.x < 0) || (p.x > d.x + 10 && velocity.x > 0) ||
        (p.y < -10 && velocity.y < 0) || (p.y > d.y + 10 && velocity.y > 0)) {
      h.emplace<Destroy>();
    }
    transform.set_rotation(transform.rotation + fixed_c::hundredth * 2);
    if (sim.collide_any(check_point(shape_flag::kSafeShield, transform.centre))) {
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
      auto& r = resolve_entity_shape<default_shape_definition<BossShot>>(h, sim);
      explode_shapes(e, r, std::nullopt, 4, to_float(transform.centre - velocity));
      h.emplace<Destroy>();
      return;
    }
    if (rotate_speed && ++timer % 8 == 0) {
      velocity = sim.rotate_compatibility(velocity, 8 * rotate_speed);
    }
  }
};
DEBUG_STRUCT_TUPLE(BossShot, timer, velocity, rotate_speed);
}  // namespace

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const cvec4& colour, fixed rotate_speed) {
  auto h = create_ship<BossShot>(sim, position);
  add_enemy_health<BossShot>(h, 0);
  h.add(BossShot{velocity, colour, rotate_speed});
  h.add(Enemy{.threat_value = 1});
  h.add(WallTag{});
}

}  // namespace ii::legacy
