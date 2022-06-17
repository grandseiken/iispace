#include "game/logic/enemy/enemy.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"

namespace ii {
namespace {

ecs::handle spawn_snake_tail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour);

struct SnakeTail : ecs::component {
  static constexpr ship_flag kShipFlags = ship_flag::kEnemy;
  static constexpr std::uint32_t kBoundingWidth = 22;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;

  using shape =
      standard_transform<geom::shape<geom::ngon{10, 4, glm::vec4{1.f}, geom::ngon_style::kPolygon,
                                                shape_flag::kDangerous | shape_flag::kWeakShield}>>;

  std::optional<ecs::entity_id> tail;
  std::optional<ecs::entity_id> head;
  std::uint32_t timer = 150;
  std::uint32_t d_timer = 0;

  void update(ecs::handle h, Transform& transform, const Render& render, SimInterface& sim) {
    static constexpr fixed z15 = fixed_c::hundredth * 15;
    transform.rotate(z15);
    if (!--timer) {
      on_destroy(transform, sim);
      h.emplace<Destroy>();
      explode_entity_shapes_with_colour<SnakeTail>(h, sim, *render.colour_override);
    }
    if (d_timer && !--d_timer) {
      if (tail && sim.index().contains(*tail)) {
        sim.index().get(*tail)->get<SnakeTail>()->d_timer = 4;
      }
      on_destroy(transform, sim);
      h.emplace<Destroy>();
      explode_entity_shapes_with_colour<SnakeTail>(h, sim, *render.colour_override);
      sim.play_sound(ii::sound::kEnemyDestroy, transform.centre, true);
    }
  }

  void on_destroy(const Transform& transform, SimInterface& sim) const {
    if (head && sim.index().contains(*head)) {
      sim.index().get(*head)->get<SnakeTail>()->tail.reset();
    }
    if (tail && sim.index().contains(*tail)) {
      sim.index().get(*tail)->get<SnakeTail>()->head.reset();
    }
  }
};

struct Snake : ecs::component {
  static constexpr ship_flag kShipFlags = ship_flag::kEnemy;
  static constexpr std::uint32_t kBoundingWidth = 32;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;

  using shape =
      standard_transform<geom::shape<geom::ngon{14, 3, glm::vec4{1.f}, geom::ngon_style::kPolygon,
                                                shape_flag::kVulnerable}>,
                         geom::shape<geom::ball_collider{10, shape_flag::kDangerous}>>;

  std::optional<ecs::entity_id> tail;
  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool is_projectile = false;
  glm::vec4 colour{0.f};
  fixed projectile_rotation = 0;

  Snake(SimInterface& sim, const glm::vec4& colour, const vec2& direction, fixed rotation)
  : colour{colour}, projectile_rotation{rotation} {
    if (direction == vec2{0}) {
      auto r = sim.random(4);
      dir = r == 0 ? vec2{1, 0} : r == 1 ? vec2{-1, 0} : r == 2 ? vec2{0, 1} : vec2{0, -1};
    } else {
      dir = normalise(direction);
      is_projectile = true;
    }
  }

  void update(ecs::handle h, Transform& transform, Render& render, SimInterface& sim) {
    if (transform.centre.x < -8 || transform.centre.x > ii::kSimDimensions.x + 8 ||
        transform.centre.y < -8 || transform.centre.y > ii::kSimDimensions.y + 8) {
      tail.reset();
      h.emplace<Destroy>();
      return;
    }

    auto c = colour;
    c.x += (timer % 256) / 256.f;
    render.colour_override = c;
    ++timer;
    if (timer % (is_projectile ? 4 : 8) == 0) {
      auto c_dark = c;
      c_dark.a = .6f;
      auto new_h = spawn_snake_tail(sim, transform.centre, c_dark);
      if (tail && sim.index().contains(*tail)) {
        sim.index().get(*tail)->get<SnakeTail>()->head = new_h.id();
        new_h.get<SnakeTail>()->tail = tail;
      }
      tail = new_h.id();
      sim.play_sound(ii::sound::kBossFire, transform.centre, true, .5f);
    }
    if (!is_projectile && timer % 48 == 0 && !sim.random(3)) {
      dir = rotate(dir, (sim.random(2) ? 1 : -1) * fixed_c::pi / 2);
      transform.set_rotation(angle(dir));
    }
    transform.move(dir * (is_projectile ? 4 : 2));
    if (timer % 8 == 0) {
      dir = rotate(dir, 8 * projectile_rotation);
      transform.set_rotation(angle(dir));
    }
  }

  void on_destroy(const Transform& transform, SimInterface& sim) const {
    if (tail) {
      sim.index().get(*tail)->get<SnakeTail>()->d_timer = 4;
    }
  }
};

ecs::handle spawn_snake_tail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour) {
  auto h = create_ship<SnakeTail>(sim, position);
  add_enemy_health<SnakeTail>(h, 0);
  h.add(SnakeTail{});
  h.add(Enemy{.threat_value = 1});
  h.get<Render>()->colour_override = colour;
  return h;
}

}  // namespace

void spawn_snake(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour,
                 const vec2& direction, fixed rotation) {
  auto h = create_ship<Snake>(sim, position);
  add_enemy_health<Snake>(h, 5);
  h.add(Snake{sim, colour, direction, rotation});
  h.add(Enemy{.threat_value = 5});
  h.get<Transform>()->set_rotation(angle(direction));
}

}  // namespace ii
