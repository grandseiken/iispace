#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

void GlobalData::pre_update(SimInterface&) {
  extra_enemy_warnings.clear();
}

void GlobalData::post_update(SimInterface& sim) {
  for (auto it = fireworks.begin(); it != fireworks.end();) {
    if (it->time > 0) {
      --(it++)->time;
      continue;
    }
    // Stupid compatibility.
    std::optional<bool> p0_has_powerup;
    sim.index().iterate<Player>([&](const Player& p) {
      if (!p0_has_powerup) {
        p0_has_powerup = p.has_bomb || p.has_shield;
      }
    });
    auto explode = [&](const glm::vec2& v, const glm::vec4& c, std::uint32_t time) {
      auto c_dark = c;
      c_dark.a = .5f;
      sim.explosion(v, c, time);
      sim.explosion(v + glm::vec2{8.f, 0.f}, c, time);
      sim.explosion(v + glm::vec2{0.f, 8.f}, c, time);
      sim.explosion(v + glm::vec2{8.f, 0.f}, c_dark, time);
      sim.explosion(v + glm::vec2{0.f, 8.f}, c_dark, time);
      if (p0_has_powerup && *p0_has_powerup) {
        sim.explosion(v, glm::vec4{0.f}, 8);
      }
    };

    explode(to_float(it->position), glm::vec4{1.f}, 16);
    explode(to_float(it->position), it->colour, 32);
    explode(to_float(it->position), glm::vec4{1.f}, 64);
    explode(to_float(it->position), it->colour, 128);
    it = fireworks.erase(it);
  }
}

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
  if (auto* c = h.get<Transform>()) {
    position = c->centre;
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
