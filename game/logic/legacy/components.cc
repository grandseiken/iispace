#include "game/logic/legacy/components.h"
#include "game/logic/sim/sim_interface.h"

namespace ii::legacy {

void PlayerScore::add(SimInterface& sim, std::uint64_t s) {
  score += s * multiplier;
  ++multiplier_count;
  if (multiplier_count >= (1u << std::min(multiplier + 3, 23u))) {
    multiplier_count = 0;
    ++multiplier;
  }
}

void GlobalData::pre_update(SimInterface& sim) {
  non_wall_enemy_count = sim.index().count<Enemy>() - sim.index().count<WallTag>();
  extra_enemy_warnings.clear();
}

void GlobalData::post_update(ecs::handle h, SimInterface& sim) {
  for (auto it = fireworks.begin(); it != fireworks.end();) {
    if (it->time > 0) {
      --(it++)->time;
      continue;
    }
    // Stupid compatibility.
    std::optional<bool> p0_has_powerup;
    auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kUnknown));
    sim.index().iterate<Player>([&](const Player& p) {
      if (!p0_has_powerup) {
        p0_has_powerup = p.bomb_count || p.shield_count;
      }
    });
    auto explode = [&](const glm::vec2& v, const glm::vec4& c, std::uint32_t time) {
      auto c_dark = c;
      c_dark.a = .5f;
      e.explosion(v, c, time);
      e.explosion(v + glm::vec2{8.f, 0.f}, c, time);
      e.explosion(v + glm::vec2{0.f, 8.f}, c, time);
      e.explosion(v + glm::vec2{8.f, 0.f}, c_dark, time);
      e.explosion(v + glm::vec2{0.f, 8.f}, c_dark, time);
      if (p0_has_powerup && *p0_has_powerup) {
        e.explosion(v, glm::vec4{0.f}, 8);
      }
    };

    explode(to_float(it->position), glm::vec4{1.f}, 16);
    explode(to_float(it->position), it->colour, 32);
    explode(to_float(it->position), glm::vec4{1.f}, 64);
    explode(to_float(it->position), it->colour, 128);
    it = fireworks.erase(it);
  }
}

}  // namespace ii::legacy