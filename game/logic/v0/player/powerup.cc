#include "game/logic/v0/player/powerup.h"
#include "game/geometry/node_conditional.h"
#include "game/geometry/shapes/ngon.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/lib/particles.h"
#include "game/logic/v0/lib/ship_template.h"
#include "game/logic/v0/player/loadout.h"
#include <algorithm>
#include <utility>

namespace ii::v0 {
namespace {
constexpr std::uint32_t kPowerupTimer = 90u * 60u;
constexpr fixed kPowerupCloseDistance = 50;
constexpr fixed kPowerupCollectDistance = 14;

float fade(std::uint32_t tick_count) {
  return (1.f + sin(static_cast<float>(tick_count) / 16.f)) / 2.f;
}

void wobble_movement(SimInterface& sim, const Transform& transform, vec2& dir,
                     std::uint32_t rotate_time, bool& first_frame, bool& rotate_anti) {
  if (!sim.is_on_screen(transform.centre)) {
    dir = sim.dimensions() / 2_fx - transform.centre;
    first_frame = false;
  } else {
    if (first_frame) {
      dir = from_polar(sim.random_fixed() * 2 * pi<fixed>, 1_fx);
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
  static constexpr std::uint32_t kRotateTime = 120;

  static constexpr auto z = colour::kZPlayerBubble;
  using shape = geom::translate_p<
      0,
      geom::rotate_eval<
          geom::multiply_p<-2_fx, 1>,
          geom::compound<geom::ngon_collider<geom::nd(14, 8), shape_flag::kVulnerable>,
                         geom::ngon_colour_p<geom::nd(16, 8), 5,
                                             geom::nline(colour::kZero, colour::kZOutline, 2.f)>,
                         geom::ngon_colour_p<geom::nd(14, 8), 4, geom::nline(colour::kZero, z)>>>,
      geom::rotate_p<
          1, geom::ngon<geom::nd(21, 3), geom::nline(colour::kOutline, colour::kZOutline, 3.f)>,
          geom::ngon_colour_p2<geom::nd(18, 3), 2, 3, geom::nline(colour::kZero, z, 1.5f),
                               geom::sfill(colour::kZero, z)>>>;
  std::tuple<vec2, fixed, cvec4, cvec4, cvec4, cvec4>
  shape_parameters(const Transform& transform) const {
    return {transform.centre,
            transform.rotation,
            v0_player_colour(player_number),
            colour::alpha(v0_player_colour(player_number), colour::kFillAlpha0),
            colour::alpha(colour::kWhite0, fade(tick_count)),
            colour::alpha(colour::kOutline, fade(tick_count))};
  }

  PlayerBubble(std::uint32_t player_number) : player_number{player_number} {}
  std::uint32_t player_number = 0;
  std::uint32_t tick_count = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;
  bool on_screen = false;

  void update(Transform& transform, SimInterface& sim) {
    tick_count = sim.tick_count();
    wobble_movement(sim, transform, dir, kRotateTime, first_frame, rotate_anti);

    auto move = normalise(dir);
    const auto& input = sim.input(player_number);
    if (on_screen && length(input.velocity) > fixed_c::hundredth) {
      auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
      move += 2 * v;
    }
    transform.rotate(pi<fixed> / 100);
    transform.move(normalise(move) * kSpeed);
    on_screen = on_screen || sim.is_on_screen(transform.centre);
  }

  void render_panel(const Transform& transform, std::vector<render::combo_panel>& output,
                    const SimInterface& sim) const {
    if (on_screen) {
      render_player_name_panel(player_number, transform, output, sim);
    }
  }
};
DEBUG_STRUCT_TUPLE(PlayerBubble, player_number, tick_count, dir, first_frame, rotate_anti);

struct ShieldPowerup : ecs::component {
  static constexpr fixed kSpeed = 3_fx / 4_fx;
  static constexpr std::uint32_t kRotateTime = 150;

  static constexpr auto z = colour::kZPowerup;
  using shape =
      standard_transform<geom::ngon_colour_p<geom::nd(14, 6), 2, geom::nline(colour::kZero, z)>,
                         geom::ngon<geom::nd(11, 6), geom::nline(colour::kWhite0, z, 1.5f)>>;

  std::tuple<vec2, fixed, cvec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colour::alpha(colour::kWhite0, fade(timer))};
  }

  ShieldPowerup() = default;
  std::uint32_t timer = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (!timer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), colour::kWhite0);
    }
    if (++timer >= kPowerupTimer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), colour::kWhite0);
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

    transform.rotate(pi<fixed> / 120);
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
    e.explosion(to_float(transform.centre), colour::kWhite0);
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
  static constexpr std::uint32_t kRotateTime = 150;

  static constexpr auto z = colour::kZPowerup;
  using shape = standard_transform<
      geom::ngon_colour_p<geom::nd(14, 6), 2, geom::nline(colour::kZero, z)>,
      geom::rotate_p<1, geom::ngon<geom::nd(5, 6), geom::nline(colour::kWhite0, z, 1.5f)>>>;

  std::tuple<vec2, fixed, cvec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colour::alpha(colour::kWhite0, fade(timer))};
  }

  BombPowerup() = default;
  std::uint32_t timer = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (!timer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), colour::kWhite0);
    }
    if (++timer >= kPowerupTimer) {
      sim.emit(resolve_key::predicted()).explosion(to_float(transform.centre), colour::kWhite0);
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

    transform.rotate(pi<fixed> / 120);
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
    e.rumble(pc.player_number, 10, .25f, .75f);
    e.explosion(to_float(transform.centre), colour::kWhite0);
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
  auto d = (1 + r.uint(12u)) * kDistance;

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
  h.add(Health{.hp = 96,
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

void render_player_name_panel(std::uint32_t player_number, const Transform& transform,
                              std::vector<render::combo_panel>& output, const SimInterface& sim) {
  static constexpr std::int32_t kPadding = 4;
  static constexpr std::int32_t kNamePanelOffsetY = 32;
  static constexpr std::uint32_t kNamePanelFontSize = 12;
  frect bounds{to_float(transform.centre) + fvec2{0, kNamePanelOffsetY},
               fvec2{0, kNamePanelFontSize + kPadding * 2}};
  output.emplace_back(render::combo_panel{
      .panel = {.style = render::panel_style::kFlatColour,
                .colour = colour::kBlackOverlay0,
                .bounds = bounds},
      .padding = ivec2{kPadding},
      .elements = {{
          .bounds = {fvec2{0}, fvec2{0, kNamePanelFontSize}},
          .e =
              render::combo_panel::text{
                  .font = {render::font_id::kMonospaceBold, uvec2{kNamePanelFontSize}},
                  .align = render::alignment::kCentered,
                  .colour = player_colour(sim.conditions().mode, player_number),
                  .drop_shadow = {{}},
                  .text = sim.conditions().players[player_number].player_name,
              },
      }},
  });
}

}  // namespace ii::v0
