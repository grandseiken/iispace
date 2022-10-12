#include "game/logic/legacy/player/powerup.h"
#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/player.h"
#include <algorithm>
#include <utility>

namespace ii::legacy {
namespace {
constexpr std::uint32_t kMagicShotCount = 120;

struct Powerup : ecs::component {
  static constexpr fixed kSpeed = 1;
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 100;
  static constexpr glm::vec4 cw{1.f};

  using out0 = geom::ngon_colour_p<13, 5, 2>;
  using out1 = geom::ngon_colour_p<9, 5, 3>;
  using extra_life = geom::switch_entry<powerup_type::kExtraLife, geom::ngon<8, 3, cw>>;
  using magic_shots = geom::switch_entry<powerup_type::kMagicShots, geom::box<3, 3, cw>>;
  using shield = geom::switch_entry<powerup_type::kShield, geom::ngon<11, 5, cw>>;
  using bomb = geom::switch_entry<powerup_type::kBomb, geom::polystar<11, 10, cw>>;
  using shape =
      standard_transform<out0, out1, geom::switch_p<4, extra_life, magic_shots, shield, bomb>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4, powerup_type>
  shape_parameters(const Transform& transform) const {
    auto c0 = player_colour((frame % (2 * kMaxPlayers)) / 2);
    auto c1 = player_colour(((frame + 1) % (2 * kMaxPlayers)) / 2);
    return {transform.centre, fixed_c::pi / 2, c0, c1, type};
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
        dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 1_fx);
      }

      dir = sim.rotate_compatibility(dir, 2 * fixed_c::hundredth * (rotate ? 1 : -1));
      rotate = sim.random(kRotateTime) ? rotate : !rotate;
    }
    first_frame = false;

    auto ph = sim.nearest_player(transform.centre);
    auto pv = ph.get<Transform>()->centre - transform.centre;
    const auto& p = *ph.get<Player>();
    if (length(pv) <= 40 && !p.is_killed()) {
      dir = pv;
    }
    dir = normalise(dir);

    transform.move(dir * kSpeed * ((length(pv) <= 40) ? 3 : 1));
    if (length(pv) <= 10 && !p.is_predicted && !p.is_killed()) {
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
      pc.magic_shot_count = kMagicShotCount;
      break;

    case powerup_type::kShield:
      pc.has_shield = true;
      pc.has_bomb = false;
      break;

    case powerup_type::kBomb:
      pc.has_bomb = true;
      pc.has_shield = false;
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
          .velocity = from_polar(random.fixed().to_float() * 2.f * glm::pi<float>(), 6.f),
          .colour = glm::vec4{1.f},
          .data = dot_particle{},
          .end_time = 4 + random.uint(8),
      });
    }
    h.emplace<Destroy>();
  }
};
DEBUG_STRUCT_TUPLE(Powerup, type, frame, dir, rotate, first_frame);

}  // namespace

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  auto h = create_ship<Powerup>(sim, position);
  h.add(Powerup{type});
  h.add(PowerupTag{.type = type});
}

}  // namespace ii::legacy
