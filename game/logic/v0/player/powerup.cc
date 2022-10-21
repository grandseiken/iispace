#include "game/logic/v0/player/powerup.h"
#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/v0/ship_template.h"
#include <algorithm>
#include <utility>

namespace ii::v0 {
namespace {

struct Powerup : ecs::component {
  static constexpr fixed kSpeed = 7_fx / 8_fx;
  static constexpr float kZIndex = -2.f;
  static constexpr std::uint32_t kRotateTime = 150;
  static constexpr glm::vec4 cw{1.f};

  using out0 = geom::ngon_colour_p<13, 5, 2>;
  using out1 = geom::ngon_colour_p<9, 5, 3>;
  using extra_life = geom::switch_entry<powerup_type::kExtraLife, geom::ngon<8, 3, cw>>;
  using magic_shots = geom::switch_entry<powerup_type::kMagicShots, geom::box<3, 3, cw>>;
  using shield = geom::switch_entry<powerup_type::kShield, geom::ngon<11, 5, cw>>;
  using bomb = geom::switch_entry<powerup_type::kBomb, geom::polystar<11, 10, cw>>;
  using shape =
      standard_transform<out0, out1, geom::switch_p<4, extra_life, magic_shots, shield, bomb>>;

  Powerup() = default;
  std::uint32_t tick_count = 0;
  vec2 dir{0};
  bool first_frame = true;
  bool rotate_anti = false;

  void update(Transform& transform, SimInterface& sim) {
    tick_count = sim.tick_count();
    if (!sim.is_on_screen(transform.centre)) {
      dir = sim.dimensions() / 2_fx - transform.centre;
    } else {
      if (first_frame) {
        dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 1_fx);
        rotate_anti = sim.random_bool();
      }

      dir = rotate(dir, 2 * fixed_c::hundredth * (rotate_anti ? 1 : -1));
      rotate_anti = sim.random(kRotateTime) ? rotate_anti : !rotate_anti;
    }
    first_frame = false;
    transform.rotate(fixed_c::pi / 120);
    transform.move(normalise(dir) * kSpeed);
  }

  void
  collect(ecs::handle h, const Transform& transform, SimInterface& sim, ecs::handle source) const {
    auto& pc = *source.get<Player>();
    auto& render = *source.get<Render>();
    auto e = sim.emit(resolve_key::local(pc.player_number));
    e.play(false ? sound::kPowerupLife : sound::kPowerupOther, transform.centre);
    e.rumble(source.get<Player>()->player_number, 10, .25f, .75f);
    e.explosion(to_float(transform.centre), glm::vec4{1.f});
    h.emplace<Destroy>();
  }
};
DEBUG_STRUCT_TUPLE(Powerup, tick_count, dir, first_frame, rotate_anti);

}  // namespace

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  auto h = create_ship<Powerup>(sim, position);
  h.add(Powerup{});
  h.add(PowerupTag{.type = type});
}

}  // namespace ii::v0
