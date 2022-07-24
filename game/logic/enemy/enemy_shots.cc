#include "game/logic/enemy/enemy.h"
#include "game/logic/geometry/shapes/shapes.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>

namespace ii {
namespace {
struct BossShot : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 12;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;

  using shape = standard_transform<geom::polystar_colour_p<16, 8, 2>, geom::ngon_colour_p<10, 8, 2>,
                                   geom::ball_collider<12, shape_flag::kDangerous>>;

  std::tuple<vec2, fixed, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colour};
  }

  BossShot(const vec2& velocity, const glm::vec4& colour, fixed rotate_speed)
  : velocity{velocity}, colour{colour}, rotate_speed{rotate_speed} {}
  std::uint32_t timer = 0;
  vec2 velocity{0};
  glm::vec4 colour{0.f};
  fixed rotate_speed = 0;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    transform.move(velocity);
    vec2 p = transform.centre;
    if ((p.x < -10 && velocity.x < 0) || (p.x > kSimDimensions.x + 10 && velocity.x > 0) ||
        (p.y < -10 && velocity.y < 0) || (p.y > kSimDimensions.y + 10 && velocity.y > 0)) {
      h.emplace<Destroy>();
    }
    transform.set_rotation(transform.rotation + fixed_c::hundredth * 2);
    if (sim.any_collision(transform.centre, shape_flag::kSafeShield)) {
      explode_entity_shapes<BossShot>(h, sim, std::nullopt, 4, transform.centre - velocity);
      h.emplace<Destroy>();
      return;
    }
    if (rotate_speed && ++timer % 8 == 0) {
      velocity = sim.rotate_compatibility(velocity, 8 * rotate_speed);
    }
  }
};
}  // namespace

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const glm::vec4& colour, fixed rotate_speed) {
  auto h = create_ship<BossShot>(sim, position);
  add_enemy_health<BossShot>(h, 0);
  h.add(BossShot{velocity, colour, rotate_speed});
  h.add(Enemy{.threat_value = 1});
  h.add(WallTag{});
}

}  // namespace ii
