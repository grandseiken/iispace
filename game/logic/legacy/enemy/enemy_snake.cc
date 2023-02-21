#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"

namespace ii::legacy {
namespace {
using namespace geom2;

ecs::handle spawn_snake_tail(SimInterface& sim, const vec2& position, const cvec4& colour);

struct SnakeTail : ecs::component {
  static constexpr float kZIndex = 11.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr fixed kBoundingWidth = 22;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kWeakShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(10, 4), .line = {.colour0 = key{'c'}}});
    n.add(ball_collider{.dimensions = bd(10), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, colour);
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
      auto& r = resolve_entity_shape2<default_shape_definition<SnakeTail>>(h, sim);
      explode_shapes(e, r, std::nullopt, 10, std::nullopt, 1.5f);
    }
    if (d_timer && !--d_timer) {
      if (tail && sim.index().contains(*tail)) {
        sim.index().get(*tail)->get<SnakeTail>()->d_timer = 4;
      }
      on_destroy(sim);
      h.emplace<Destroy>();
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
      auto& r = resolve_entity_shape2<default_shape_definition<SnakeTail>>(h, sim);
      explode_shapes(e, r, std::nullopt, 10, std::nullopt, 1.5f);
      destruct_lines(e, r, to_float(transform.centre));
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
  static constexpr float kZIndex = 12.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kMedium;
  static constexpr fixed kBoundingWidth = 32;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(14, 3), .line = {.colour0 = key{'c'}}});
    n.add(ball_collider{.dimensions = bd(14), .flags = shape_flag::kVulnerable});
    n.add(ball_collider{.dimensions = bd(10), .flags = shape_flag::kDangerous});
    n.add(/* dummy */ ball{.line = {.colour0 = key{'c'}}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, colour);
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
  auto h = create_ship2<SnakeTail>(sim, position);
  add_enemy_health2<SnakeTail>(h, 0);
  h.add(SnakeTail{colour});
  h.add(Enemy{.threat_value = 1});
  return h;
}

}  // namespace

void spawn_snake(SimInterface& sim, const vec2& position, const cvec4& colour,
                 const vec2& direction, fixed rotation) {
  auto h = create_ship2<Snake>(sim, position);
  add_enemy_health2<Snake>(h, 5);
  h.add(Snake{sim, colour, direction, rotation});
  h.add(Enemy{.threat_value = 5});
  h.get<Transform>()->set_rotation(angle(direction));
}

}  // namespace ii::legacy
