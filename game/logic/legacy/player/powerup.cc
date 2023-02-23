#include "game/logic/legacy/player/powerup.h"
#include "game/logic/legacy/components.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include <algorithm>
#include <utility>

namespace ii::legacy {
namespace {
using namespace geom;
constexpr std::uint32_t kMagicShotCount = 120;

struct Powerup : ecs::component {
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 100;
  static constexpr fixed kSpeed = 1;
  static constexpr fixed kBoundingWidth = 0;
  static constexpr auto kFlags = shape_flag::kNone;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = pi<fixed> / 2});
    n.add(ngon{.dimensions = nd(13, 5), .line = {.colour0 = key{'c'}}});
    n.add(ngon{.dimensions = nd(9, 5), .line = {.colour0 = key{'C'}}});
    n.add(enable{compare(root, static_cast<std::uint32_t>(powerup_type::kExtraLife), key{'t'})})
        .add(ngon{.dimensions = nd(8, 3), .line = {.colour0 = colour::kWhite0}});
    n.add(enable{compare(root, static_cast<std::uint32_t>(powerup_type::kMagicShots), key{'t'})})
        .add(box{.dimensions = vec2{3}, .line = {.colour0 = colour::kWhite0}});
    n.add(enable{compare(root, static_cast<std::uint32_t>(powerup_type::kShield), key{'t'})})
        .add(ngon{.dimensions = nd(11, 5), .line = {.colour0 = colour::kWhite0}});
    n.add(enable{compare(root, static_cast<std::uint32_t>(powerup_type::kBomb), key{'t'})})
        .add(ngon{.dimensions = nd(11, 10),
                  .style = ngon_style::kPolystar,
                  .line = {.colour0 = colour::kWhite0}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    auto c0 = legacy_player_colour((frame % (2 * kMaxPlayers)) / 2);
    auto c1 = legacy_player_colour(((frame + 1) % (2 * kMaxPlayers)) / 2);
    parameters.add(key{'v'}, transform.centre)
        .add(key{'c'}, c0)
        .add(key{'C'}, c1)
        .add(key{'t'}, type);
  }

  Powerup(powerup_type type) : type{type} {}
  powerup_type type = powerup_type::kExtraLife;
  std::uint32_t frame = 0;
  vec2 dir = {0, 1};
  bool rotate = false;
  bool first_frame = true;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    ++frame;

    if (!sim.is_on_screen(transform.centre)) {
      dir = sim.dimensions() / 2_fx - transform.centre;
    } else {
      if (first_frame) {
        dir = from_polar_legacy(sim.random_fixed() * 2 * pi<fixed>, 1_fx);
      }

      dir = sim.rotate_compatibility(dir, 2 * fixed_c::hundredth * (rotate ? 1 : -1));
      rotate = sim.random(kRotateTime) ? rotate : !rotate;
    }
    first_frame = false;

    auto ph = sim.nearest_player(transform.centre);
    auto pv = ph.get<Transform>()->centre - transform.centre;
    const auto& p = *ph.get<Player>();
    if (length(pv) <= 40 && !p.is_killed) {
      dir = pv;
    }
    dir = normalise(dir);

    transform.move(dir * kSpeed * ((length(pv) <= 40) ? 3 : 1));
    if (length(pv) <= 10 && !p.is_predicted && !p.is_killed) {
      collect(h, transform, sim, ph);
    }
  }

  void
  collect(ecs::handle h, const Transform& transform, SimInterface& sim, ecs::handle source) const {
    auto& pc = *source.get<Player>();
    auto e = sim.emit(resolve_key::local(pc.player_number));
    switch (type) {
    case powerup_type::kExtraLife:
      ++sim.global_entity().get<GlobalData>()->lives;
      break;

    case powerup_type::kMagicShots:
      pc.super_charge = kMagicShotCount;
      break;

    case powerup_type::kShield:
      pc.shield_count = 1;
      pc.bomb_count = 0;
      break;

    case powerup_type::kBomb:
      pc.bomb_count = 1;
      pc.shield_count = 0;
      break;
    }
    e.play(type == powerup_type::kExtraLife ? sound::kPowerupLife : sound::kPowerupOther,
           transform.centre);
    e.rumble(source.get<Player>()->player_number, 10, .25f, .75f);

    auto& random = sim.random(random_source::kLegacyAesthetic);
    auto r = 5 + random.uint(5);
    for (std::uint32_t i = 0; i < r; ++i) {
      e.add(particle{
          .position = to_float(transform.centre),
          .velocity = from_polar(random.fixed().to_float() * 2.f * pi<float>, 6.f),
          .colour = cvec4{1.f},
          .data = dot_particle{},
          .end_time = 4 + random.uint(8),
      });
    }
    h.emplace<Destroy>();
  }

  bool ai_requires(const SimInterface& sim, ecs::const_handle ph) const {
    const auto& pc = *ph.get<Player>();
    if ((type == powerup_type::kBomb || type == powerup_type::kShield) &&
        (pc.bomb_count || pc.shield_count)) {
      return false;
    }
    if (type == powerup_type::kMagicShots && pc.super_charge) {
      return false;
    }
    return true;
  }
};
DEBUG_STRUCT_TUPLE(Powerup, type, frame, dir, rotate, first_frame);

}  // namespace

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  auto h = create_ship<Powerup>(sim, position);
  h.add(Powerup{type});
  h.add(PowerupTag{.ai_requires = ecs::call<&Powerup::ai_requires>});
}

}  // namespace ii::legacy
