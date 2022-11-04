#include "game/logic/v0/player/powerup.h"
#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/ship_template.h"
#include <algorithm>
#include <utility>

namespace ii::v0 {
namespace {
constexpr std::uint32_t kPowerupTimer = 3600;
constexpr fixed kPowerupCloseDistance = 50;
constexpr fixed kPowerupCollectDistance = 10;

glm::vec4 fade(std::uint32_t tick_count) {
  auto lightness = (1.f + sin(static_cast<float>(tick_count) / 16.f)) / 2.f;
  return {1.f, 1.f, 1.f, lightness};
}

void wobble_movement(SimInterface& sim, const Transform& transform, vec2& dir,
                     std::uint32_t rotate_time, bool& first_frame, bool& rotate_anti) {
  if (!sim.is_on_screen(transform.centre)) {
    dir = sim.dimensions() / 2_fx - transform.centre;
    first_frame = true;
  } else {
    if (first_frame) {
      dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 1_fx);
      rotate_anti = sim.random_bool();
    }

    dir = rotate(dir, 2 * fixed_c::hundredth * (rotate_anti ? 1 : -1));
    rotate_anti = sim.random(rotate_time) ? rotate_anti : !rotate_anti;
    first_frame = false;
  }
}

struct PlayerBubble : ecs::component {
  static constexpr fixed kSpeed = 1;
  static constexpr fixed kBoundingWidth = 16;
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 120;

  using shape = geom::translate_p<
      0,
      geom::rotate_eval<geom::multiply_p<-2_fx, 1>,
                        geom::polygon_colour_p<14, 8, 3, shape_flag::kVulnerable>>,
      geom::rotate_p<1, geom::polygon_colour_p<18, 3, 2>>>;
  std::tuple<vec2, fixed, glm::vec4, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, player_colour(player_number), fade(tick_count)};
  }

  PlayerBubble(std::uint32_t player_number) : player_number{player_number} {}
  std::uint32_t player_number = 0;
  std::uint32_t tick_count = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(Transform& transform, SimInterface& sim) {
    tick_count = sim.tick_count();
    wobble_movement(sim, transform, dir, kRotateTime, first_frame, rotate_anti);

    transform.rotate(fixed_c::pi / 100);
    transform.move(normalise(dir) * kSpeed);
  }
};
DEBUG_STRUCT_TUPLE(PlayerBubble, player_number, tick_count, dir, first_frame, rotate_anti);

struct ShieldPowerup : ecs::component {
  static constexpr fixed kSpeed = 3_fx / 4_fx;
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 150;

  using shape =
      standard_transform<geom::polygon_colour_p<14, 6, 2>, geom::polygon<11, 6, glm::vec4{1.f}>>;

  std::tuple<vec2, fixed, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, fade(timer)};
  }

  ShieldPowerup() = default;
  std::uint32_t timer = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (++timer >= kPowerupTimer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), glm::vec4{1.f});
      h.emplace<Destroy>();
      return;
    }
    wobble_movement(sim, transform, dir, kRotateTime, first_frame, rotate_anti);

    auto ph = sim.nearest_player(transform.centre);
    auto pv = ph.get<Transform>()->centre - transform.centre;
    const auto& p = *ph.get<Player>();
    bool required = !p.is_killed && is_required(sim, ph) &&
        length_squared(pv) <= kPowerupCloseDistance * kPowerupCloseDistance;
    if (required) {
      dir = pv;
    }
    dir = normalise(dir);

    transform.rotate(fixed_c::pi / 120);
    transform.move(dir * kSpeed * (required ? 3 : 1));
    if (required && !p.is_predicted &&
        length_squared(pv) <= kPowerupCollectDistance * kPowerupCollectDistance) {
      collect(h, transform, sim, ph);
    }
  }

  void
  collect(ecs::handle h, const Transform& transform, SimInterface& sim, ecs::handle source) const {
    auto& pc = *source.get<Player>();
    auto e = sim.emit(resolve_key::local(pc.player_number));
    e.play(sound::kPowerupOther, transform.centre);
    e.rumble(source.get<Player>()->player_number, 10, .25f, .75f);
    e.explosion(to_float(transform.centre), glm::vec4{1.f});
    h.emplace<Destroy>();
    ++pc.shield_count;
  }

  bool is_required(const SimInterface& sim, ecs::const_handle ph) const {
    const auto& pc = *ph.get<Player>();
    const auto& loadout = *ph.get<PlayerLoadout>();
    return pc.shield_count < loadout.max_shield_capacity(sim);
  }
};
DEBUG_STRUCT_TUPLE(ShieldPowerup, timer, dir, first_frame, rotate_anti);

struct BombPowerup : ecs::component {
  static constexpr fixed kSpeed = 3_fx / 4_fx;
  static constexpr fixed kEdgeDistance = 32_fx;
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 150;

  using shape = standard_transform<geom::polygon_colour_p<14, 6, 2>,
                                   geom::rotate_p<1, geom::polygon<5, 6, glm::vec4{1.f}>>>;

  std::tuple<vec2, fixed, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, fade(timer)};
  }

  BombPowerup() = default;
  std::uint32_t timer = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (++timer >= kPowerupTimer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), glm::vec4{1.f});
      h.emplace<Destroy>();
      return;
    }
    wobble_movement(sim, transform, dir, kRotateTime, first_frame, rotate_anti);

    vec2 evade_dir{0};
    std::uint32_t evade_count = 0;
    sim.index().iterate_dispatch<Player>([&](const Player& pc, const Transform& p_transform) {
      if (!pc.is_killed) {
        evade_dir += normalise(p_transform.centre - transform.centre);
        ++evade_count;
      }
    });
    if (evade_count > 1) {
      evade_dir /= evade_count;
    }
    if (angle_diff(angle(dir), angle(evade_dir)) > 0 == rotate_anti &&
        !sim.random(kRotateTime / 2)) {
      rotate_anti = !rotate_anti;
    }

    auto ph = sim.nearest_player(transform.centre);
    auto pv = ph.get<Transform>()->centre - transform.centre;
    const auto& p = *ph.get<Player>();
    bool required = !p.is_killed && is_required(sim, ph) &&
        length_squared(pv) <= kPowerupCloseDistance * kPowerupCloseDistance;
    if (required) {
      dir = pv;
    }
    dir = normalise(dir);

    transform.rotate(fixed_c::pi / 120);
    transform.move(dir * kSpeed * (required ? 3 : 1));
    if (required && !p.is_predicted &&
        length_squared(pv) <= kPowerupCollectDistance * kPowerupCollectDistance) {
      collect(h, transform, sim, ph);
    }
  }

  void
  collect(ecs::handle h, const Transform& transform, SimInterface& sim, ecs::handle source) const {
    auto& pc = *source.get<Player>();
    auto e = sim.emit(resolve_key::local(pc.player_number));
    e.play(sound::kPowerupOther, transform.centre);
    e.rumble(source.get<Player>()->player_number, 10, .25f, .75f);
    e.explosion(to_float(transform.centre), glm::vec4{1.f});
    h.emplace<Destroy>();
    ++pc.bomb_count;
  }

  bool is_required(const SimInterface& sim, ecs::const_handle ph) const {
    const auto& pc = *ph.get<Player>();
    const auto& loadout = *ph.get<PlayerLoadout>();
    return pc.bomb_count < loadout.max_bomb_capacity(sim);
  }
};
DEBUG_STRUCT_TUPLE(BombPowerup, timer, dir, first_frame, rotate_anti);

}  // namespace

ecs::handle spawn_player_bubble(SimInterface& sim, ecs::handle player) {
  static constexpr auto kDistance = 24_fx;

  auto dim = sim.dimensions();
  auto position = dim / 2;

  auto& r = sim.random(random_source::kGameSequence);
  auto side = r.uint(4u);
  auto d = (1 + r.uint(8u)) * kDistance;

  if (side == 0) {
    position = {dim.x / 2, -d};
  } else if (side == 1) {
    position = {dim.x / 2, dim.y + d};
  } else if (side == 2) {
    position = {-d, dim.y / 2};
  } else {
    position = {dim.x + d, dim.y / 2};
  }

  auto h = create_ship_default<PlayerBubble>(sim, position);
  h.add(PlayerBubble{player.get<Player>()->player_number});
  h.add(Health{.hp = 100,
               .destroy_sound = sound::kPlayerDestroy,
               .destroy_rumble = rumble_type::kLarge,
               .on_destroy =
                   sfn::cast<Health::on_destroy_t, &destruct_entity_default<PlayerBubble>>});
  h.add(AiFocusTag{.priority = 24});
  return h;
}

void spawn_shield_powerup(SimInterface& sim, const vec2& position) {
  auto h = create_ship_default<ShieldPowerup>(sim, position);
  h.add(ShieldPowerup{});
  h.add(PowerupTag{.ai_requires = ecs::call<&ShieldPowerup::is_required>});
}

void spawn_bomb_powerup(SimInterface& sim, const vec2& position) {
  auto h = create_ship_default<BombPowerup>(sim, position);
  h.add(BombPowerup{});
  h.add(PowerupTag{.ai_requires = ecs::call<&BombPowerup::is_required>});
}

}  // namespace ii::v0
