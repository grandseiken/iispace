#include "game/logic/ship/components.h"
#include "game/logic/ship/ship.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

void Health::damage(ecs::handle h, SimInterface& sim, std::uint32_t damage, damage_type type,
                    std::optional<ecs::entity_id> source) {
  if (damage_transform) {
    damage = damage_transform(h, sim, type, damage);
    if (!damage) {
      return;
    }
  }
  if (on_hit) {
    on_hit(h, sim, type);
  }

  hp = hp < damage ? 0 : hp - damage;
  vec2 position = {kSimDimensions.x / 2, kSimDimensions.y / 2};
  if (auto* c = h.get<Transform>(); c) {
    position = h.get<Transform>()->centre;
  } else if (auto* c = h.get<LegacyShip>(); c) {
    position = c->ship->position();
  }

  if (hit_sound0 && damage) {
    sim.play_sound(*hit_sound0, position, /* random */ true);
  }
  if (h.has<Destroy>()) {
    return;
  }

  if (!hp) {
    if (destroy_sound) {
      sim.play_sound(*destroy_sound, position, /* random */ true);
    }
    if (on_destroy) {
      on_destroy(h, sim, type);
    }
    h.add(Destroy{.source = source});
  } else {
    if (hit_sound1 && damage) {
      sim.play_sound(*hit_sound1, position, /* random */ true);
    }
    hit_timer = std::max<std::uint32_t>(hit_timer, type == damage_type::kBomb ? 25 : 1);
  }
}

void Player::add_score(SimInterface& sim, std::uint64_t s) {
  sim.rumble(player_number, 3);
  score += s * multiplier;
  ++multiplier_count;
  if (multiplier_count >= (1u << std::min(multiplier + 3, 23u))) {
    multiplier_count = 0;
    ++multiplier;
  }
}

}  // namespace ii