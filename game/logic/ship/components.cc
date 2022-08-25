#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

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
        p0_has_powerup = p.has_bomb || p.has_shield;
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

void Health::damage(ecs::handle h, SimInterface& sim, std::uint32_t damage, damage_type type,
                    ecs::entity_id source_id, std::optional<vec2> source_position) {
  if (damage_transform) {
    damage = damage_transform(h, sim, type, damage);
    if (!damage) {
      return;
    }
  }

  auto get_source_position = [&]() -> vec2 {
    if (source_position) {
      return *source_position;
    }
    if (auto* t = sim.index().get<Transform>(source_id); t) {
      return t->centre;
    }
    if (auto* t = h.get<Transform>(); t) {
      return t->centre;
    }
    return kSimDimensions / 2;
  };

  auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnHit, hp));
  if (on_hit) {
    on_hit(h, sim, e, type, get_source_position());
  }

  if (type != damage_type::kPredicted) {
    hp = hp < damage ? 0 : hp - damage;
  }
  vec2 position = {kSimDimensions.x / 2, kSimDimensions.y / 2};
  if (auto* c = h.get<Transform>()) {
    position = c->centre;
  }

  if (hit_sound0 && damage) {
    e.play_random(*hit_sound0, position);
  }
  if (h.has<Destroy>()) {
    return;
  }

  if (!hp) {
    auto e_destroy = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
    if (destroy_sound) {
      e_destroy.play_random(*destroy_sound, position);
    }
    if (on_destroy) {
      on_destroy(h, sim, e_destroy, type, get_source_position());
    }
    h.add(Destroy{.source = source_id, .destroy_type = type});
  } else {
    if (hit_sound1 && damage) {
      e.play_random(*hit_sound1, position);
    }
    hit_timer = std::max<std::uint32_t>(hit_timer, type == damage_type::kBomb ? 25 : 1);
  }
}

void Player::add_score(SimInterface& sim, std::uint64_t s) {
  sim.emit(resolve_key::predicted()).rumble(player_number, 3);
  score += s * multiplier;
  ++multiplier_count;
  if (multiplier_count >= (1u << std::min(multiplier + 3, 23u))) {
    multiplier_count = 0;
    ++multiplier;
  }
}

}  // namespace ii
