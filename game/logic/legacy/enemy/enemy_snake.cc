#include "game/geometry/legacy/ball_collider.h"
#include "game/geometry/legacy/ngon.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"

namespace ii::legacy {
namespace {

ecs::handle spawn_snake_tail(SimInterface& sim, const vec2& position, const cvec4& colour);

struct SnakeTail : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 22;
  static constexpr float kZIndex = 11.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;

  using shape = standard_transform<
      geom::polygon_colour_p<10, 4, 2, 0, shape_flag::kDangerous | shape_flag::kWeakShield>>;

  std::tuple<vec2, fixed, cvec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colour};
  }

  SnakeTail(const cvec4& colour) : colour{colour} {}
  std::optional<ecs::entity_id> tail;
  std::optional<ecs::entity_id> head;
  std::uint32_t timer = 150;
  std::uint32_t d_timer = 0;
  cvec4 colour{0.f};

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    static constexpr fixed z15 = fixed_c::hundredth * 15;
    transform.rotate(z15);
    if (!--timer) {
      on_destroy(sim);
      h.emplace<Destroy>();
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
      explode_entity_shapes<SnakeTail>(h, e, std::nullopt, 10, std::nullopt, 1.5f);
    }
    if (d_timer && !--d_timer) {
      if (tail && sim.index().contains(*tail)) {
        sim.index().get(*tail)->get<SnakeTail>()->d_timer = 4;
      }
      on_destroy(sim);
      h.emplace<Destroy>();
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
      explode_entity_shapes<SnakeTail>(h, e, std::nullopt, 10, std::nullopt, 1.5f);
      destruct_entity_lines<SnakeTail>(h, e, transform.centre);
      e.play_random(sound::kEnemyDestroy, transform.centre);
    }
  }

  void on_destroy(SimInterface& sim) const {
    if (head && sim.index().contains(*head)) {
      sim.index().get(*head)->get<SnakeTail>()->tail.reset();
    }
    if (tail && sim.index().contains(*tail)) {
      sim.index().get(*tail)->get<SnakeTail>()->head.reset();
    }
  }
};
DEBUG_STRUCT_TUPLE(SnakeTail, tail, head, timer, d_timer);

struct Snake : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 32;
  static constexpr float kZIndex = 12.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kMedium;

  using shape = standard_transform<geom::polygon_colour_p<14, 3, 2, 0, shape_flag::kVulnerable>,
                                   geom::ball_collider<10, shape_flag::kDangerous>>;

  std::tuple<vec2, fixed, cvec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colour};
  }

  std::optional<ecs::entity_id> tail;
  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool is_projectile = false;
  cvec4 start_colour{0.f};
  cvec4 colour{0.f};
  fixed projectile_rotation = 0;

  Snake(SimInterface& sim, const cvec4& colour, const vec2& direction, fixed rotation)
  : start_colour{colour}, colour{colour}, projectile_rotation{rotation} {
    if (direction == vec2{0}) {
      auto r = sim.random(4);
      dir = r == 0 ? vec2{1, 0} : r == 1 ? vec2{-1, 0} : r == 2 ? vec2{0, 1} : vec2{0, -1};
    } else {
      dir = normalise(direction);
      is_projectile = true;
    }
  }

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    auto d = sim.dimensions();
    if (transform.centre.x < -8 || transform.centre.x > d.x + 8 || transform.centre.y < -8 ||
        transform.centre.y > d.y + 8) {
      tail.reset();
      h.emplace<Destroy>();
      return;
    }

    colour = start_colour;
    colour.x += static_cast<float>(timer % 256) / 256.f;
    ++timer;
    if (timer % (is_projectile ? 4 : 8) == 0) {
      auto c_dark = colour;
      c_dark.a = .6f;
      auto new_h = spawn_snake_tail(sim, transform.centre, c_dark);
      if (tail && sim.index().contains(*tail)) {
        sim.index().get(*tail)->get<SnakeTail>()->head = new_h.id();
        new_h.get<SnakeTail>()->tail = tail;
      }
      tail = new_h.id();
      sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre, .5f);
    }
    if (!is_projectile && timer % 48 == 0 && !sim.random(3)) {
      dir = sim.rotate_compatibility(dir, (sim.random_bool() ? 1 : -1) * pi<fixed> / 2);
      transform.set_rotation(angle(dir));
    }
    transform.move(dir * (is_projectile ? 4 : 2));
    if (timer % 8 == 0) {
      dir = sim.rotate_compatibility(dir, 8 * projectile_rotation);
      transform.set_rotation(angle(dir));
    }
  }

  void on_destroy(SimInterface& sim) const {
    if (tail) {
      sim.index().get(*tail)->get<SnakeTail>()->d_timer = 4;
    }
  }
};
DEBUG_STRUCT_TUPLE(Snake, tail, timer, dir, count, is_projectile, projectile_rotation);

ecs::handle spawn_snake_tail(SimInterface& sim, const vec2& position, const cvec4& colour) {
  auto h = create_ship<SnakeTail>(sim, position);
  add_enemy_health<SnakeTail>(h, 0);
  h.add(SnakeTail{colour});
  h.add(Enemy{.threat_value = 1});
  return h;
}

}  // namespace

void spawn_snake(SimInterface& sim, const vec2& position, const cvec4& colour,
                 const vec2& direction, fixed rotation) {
  auto h = create_ship<Snake>(sim, position);
  add_enemy_health<Snake>(h, 5);
  h.add(Snake{sim, colour, direction, rotation});
  h.add(Enemy{.threat_value = 5});
  h.get<Transform>()->set_rotation(angle(direction));
}

}  // namespace ii::legacy
