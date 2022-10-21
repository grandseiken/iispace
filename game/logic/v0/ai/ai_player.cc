#include "game/logic/v0/ai/ai_player.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"

namespace ii::v0 {
namespace {

struct AiPlayer : ecs::component {
  vec2 velocity{0};
  input_frame think(ecs::const_handle h, const Transform& transform, const Player& player,
                    const SimInterface& sim);
};
DEBUG_STRUCT_TUPLE(AiPlayer, velocity);

input_frame AiPlayer::think(ecs::const_handle h, const Transform& transform, const Player& player,
                            const SimInterface& sim) {
  struct target {
    fixed distance = 0;
    vec2 position{0};
    ecs::entity_id id{0};
  };

  std::optional<target> closest_enemy;
  std::optional<target> closest_wall;

  bool avoid_urgent = false;
  std::optional<vec2> avoid_v;
  std::optional<vec2> attract_v;
  std::optional<vec2> powerup_attract_v;
  std::optional<vec2> wall_attract_v;

  static constexpr fixed kAvoidDistance = 48;
  static constexpr fixed kEdgeAvoidDistance = 96;
  static constexpr fixed kAttractDistance = 128;

  input_frame frame;
  auto handle_target = [&](ecs::const_handle eh, const Enemy* enemy, const AiFocusTag* ai_focus,
                           const Transform& e_transform, const Collision* collision,
                           const Health* health, const Boss* boss, const WallTag* wall) {
    fixed size = 0;
    if (collision) {
      // TODO: necessary for ghost boss.
      size = std::min<fixed>(collision->bounding_width, 160u);
    }
    bool on_screen = false;
    for (const auto& v :
         {vec2{size, size}, vec2{size, -size}, vec2{-size, size}, vec2{-size, -size}}) {
      if (sim.is_on_screen(e_transform.centre + v)) {
        on_screen = true;
      }
    }

    auto offset = e_transform.centre - transform.centre;
    auto distance = length(offset);
    if (on_screen && health && health->hp > 0) {
      if ((!wall || distance < kAvoidDistance + size) &&
          (!closest_enemy || distance < closest_enemy->distance)) {
        closest_enemy = target{distance, e_transform.centre, eh.id()};
      }
      if (wall && (!closest_wall || distance < closest_wall->distance)) {
        closest_wall = target{distance, e_transform.centre, eh.id()};
      }
    }

    if (!distance) {
      return;
    }
    auto threat_c =
        enemy ? enemy->threat_value * enemy->threat_value : ai_focus->priority * ai_focus->priority;
    if (enemy && distance < kAvoidDistance + size) {
      if (!avoid_v) {
        avoid_v = vec2{0};
      }
      *avoid_v -= offset / (distance * distance);
      if (distance < kAvoidDistance / 2 + size / 2) {
        avoid_urgent = true;
      }
    } else if (((on_screen && !wall && !boss) || (!on_screen && boss)) &&
               distance > kAttractDistance + size) {
      if (!attract_v) {
        attract_v = vec2{0};
      }
      *attract_v += threat_c * offset / (distance * distance);
    } else if (on_screen && wall && distance > kAttractDistance + size) {
      if (!wall_attract_v) {
        wall_attract_v = vec2{0};
      }
      *wall_attract_v += threat_c * offset / (distance * distance);
    }
    if (on_screen && boss && distance < kAttractDistance + size) {
      frame.keys |= input_frame::key::kBomb;
    }
  };

  sim.index().iterate_dispatch_if<Enemy>(handle_target);
  sim.index().iterate_dispatch_if<AiFocusTag>(handle_target);

  sim.index().iterate_dispatch_if<PowerupTag>(
      [&](const PowerupTag& powerup, const Transform& p_transform) {
        if (!sim.is_on_screen(p_transform.centre)) {
          return;
        }
        auto offset = p_transform.centre - transform.centre;
        auto distance = length(offset);
        if ((powerup.type == powerup_type::kBomb || powerup.type == powerup_type::kShield) &&
            (player.has_bomb || player.has_shield)) {
          if (distance < kAvoidDistance && !avoid_v) {
            avoid_v = -offset / (distance * distance);
          }
          return;
        }
        if (powerup.type == powerup_type::kMagicShots && player.magic_shot_count) {
          return;
        }
        if (!powerup_attract_v) {
          powerup_attract_v = vec2{0};
        }
        *powerup_attract_v += offset / (distance * distance);
      });

  // Avoid other players.
  sim.index().iterate_dispatch_if<Player>([&](ecs::const_handle ph, const Transform& e_transform) {
    auto offset = e_transform.centre - transform.centre;
    auto distance = length(offset);
    if (ph.id() != h.id() && distance < kAvoidDistance && !avoid_v) {
      avoid_v = -offset / (distance * distance);
    }
  });

  if (closest_enemy) {
    frame.target_absolute = closest_enemy->position;
    frame.keys |= input_frame::key::kFire;
  } else if (closest_wall) {
    frame.target_absolute = closest_wall->position;
    frame.keys |= input_frame::key::kFire;
  }
  vec2 target_velocity{0};
  if (avoid_v) {
    target_velocity = normalise(*avoid_v);
  } else if (attract_v) {
    target_velocity = normalise(*attract_v);
  } else if (powerup_attract_v) {
    target_velocity = normalise(*powerup_attract_v);
  } else if (wall_attract_v) {
    target_velocity = normalise(*wall_attract_v);
  }

  auto dim = sim.dimensions();
  bool l_edge = transform.centre.x < kEdgeAvoidDistance && target_velocity.x < 0;
  bool r_edge = transform.centre.x >= dim.x - kEdgeAvoidDistance && target_velocity.x > 0;
  bool u_edge = transform.centre.y < kEdgeAvoidDistance && target_velocity.y < 0;
  bool d_edge = transform.centre.y >= dim.y - kEdgeAvoidDistance && target_velocity.y > 0;
  if (avoid_v && l_edge && u_edge) {
    if (abs(target_velocity.x) > abs(target_velocity.y)) {
      target_velocity.y = 1;
    } else {
      target_velocity.x = 1;
    }
  } else if (avoid_v && l_edge && d_edge) {
    if (abs(target_velocity.x) > abs(target_velocity.y)) {
      target_velocity.y = -1;
    } else {
      target_velocity.x = 1;
    }
  } else if (avoid_v && r_edge && u_edge) {
    if (abs(target_velocity.x) > abs(target_velocity.y)) {
      target_velocity.y = 1;
    } else {
      target_velocity.x = -1;
    }
  } else if (avoid_v && r_edge && d_edge) {
    if (abs(target_velocity.x) > abs(target_velocity.y)) {
      target_velocity.y = -1;
    } else {
      target_velocity.x = -1;
    }
  } else if (avoid_v && (l_edge || r_edge)) {
    target_velocity.y = target_velocity.y < 0 ? -1 : 1;
  } else if (avoid_v && (u_edge || d_edge)) {
    target_velocity.x = target_velocity.x < 0 ? -1 : 1;
  }
  if (!avoid_urgent && (l_edge || r_edge)) {
    target_velocity.x = 0;
  }
  if (!avoid_urgent && (u_edge || d_edge)) {
    target_velocity.y = 0;
  }

  if (avoid_urgent) {
    velocity = rc_smooth(velocity, target_velocity, 3_fx / 4_fx);
    frame.keys |= input_frame::key::kBomb;
  } else {
    velocity = rc_smooth(velocity, target_velocity, 15_fx / 16_fx);
  }
  frame.velocity = velocity;

  return frame;
}

}  // namespace

void add_ai(ecs::handle h) {
  h.emplace<AiPlayer>();
}

std::optional<input_frame> ai_think(const SimInterface& sim, ecs::handle h) {
  if (auto* ai = h.get<AiPlayer>(); ai) {
    return ecs::call<&AiPlayer::think>(h, sim);
  }
  return std::nullopt;
}

}  // namespace ii::v0
