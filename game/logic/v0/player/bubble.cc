#include "game/logic/v0/player/bubble.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

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
    auto lightness = (1.f + sin(static_cast<float>(tick_count) / 16.f)) / 2.f;
    return {transform.centre, transform.rotation, player_colour(player_number),
            glm::vec4{1.f, 1.f, 1.f, lightness}};
  }

  PlayerBubble(std::uint32_t player_number) : player_number{player_number} {}
  std::uint32_t player_number = 0;
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
    transform.rotate(fixed_c::pi / 100);
    transform.move(normalise(dir) * kSpeed);
  }
};
DEBUG_STRUCT_TUPLE(PlayerBubble, player_number, tick_count, dir, first_frame, rotate_anti);

}  // namespace

ecs::handle spawn_player_bubble(SimInterface& sim, ecs::handle player) {
  static constexpr auto kDistance = 24_fx;

  auto dim = sim.dimensions();
  auto position = dim / 2;
  auto r = sim.random(4u);
  if (r == 0) {
    position = {dim.x / 2, -kDistance};
  } else if (r == 1) {
    position = {dim.x / 2, dim.y + kDistance};
  } else if (r == 2) {
    position = {-kDistance, dim.y / 2};
  } else {
    position = {dim.x + kDistance, dim.y / 2};
  }

  auto h = create_ship_default<PlayerBubble>(sim, position);
  h.add(PlayerBubble{player.get<Player>()->player_number});
  h.add(Health{.hp = 12,
               .destroy_sound = sound::kPlayerDestroy,
               .destroy_rumble = rumble_type::kLarge,
               .on_destroy =
                   sfn::cast<Health::on_destroy_t, &destruct_entity_default<PlayerBubble>>});
  h.add(AiFocusTag{.priority = 24});
  return h;
}

}  // namespace ii::v0
