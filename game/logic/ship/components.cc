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

void Render::render_shapes(ecs::const_handle h, bool paused, std::vector<render::shape>& output,
                           const SimInterface& sim) {
  auto start = output.size();
  render(h, output, sim);
  auto count = output.size() - start;
  trails.resize(count);
  for (std::size_t i = 0; i < count; ++i) {
    auto& s = output[start + i];
    if (s.disable_trail) {
      s.trail.reset();
      trails[i].reset();
    } else if (trails[i]) {
      s.trail = *trails[i];
    }
    if (!paused) {
      trails[i] = render::motion_trail{
          .prev_origin = s.origin, .prev_rotation = s.rotation, .prev_colour = s.colour};
    }
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
    return sim.dimensions() / 2;
  };

  const auto* pc = sim.index().get<Player>(source_id);
  auto source_v = get_source_position();
  auto tick = sim.tick_count();
  if (pc) {
    if (pc->player_number >= hits.size()) {
      hits.resize(pc->player_number + 1);
    }
    auto& hit = hits[pc->player_number];
    hit.source = source_v;
    hit.tick = tick;
  }

  auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnHit, hp));
  if (on_hit) {
    on_hit(h, sim, e, type, source_v);
  }

  if (type != damage_type::kPredicted) {
    hp = hp < damage ? 0 : hp - damage;
  }
  auto position = sim.dimensions() / 2;
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
    static constexpr std::uint64_t kRecentHitMaxTicks = 15u;

    auto e_destroy = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy));
    if (destroy_sound) {
      e_destroy.play_random(*destroy_sound, position);
    }
    if (pc && destroy_rumble.value_or(rumble_type::kNone) != rumble_type::kNone) {
      std::uint32_t time_ticks = 0;
      float lf = 0.f;
      float hf = 0.f;
      switch (*destroy_rumble) {
      case rumble_type::kNone:
        break;
      case rumble_type::kLow:
        time_ticks = 10;
        lf = .5f;
        break;
      case rumble_type::kSmall:
        time_ticks = 6;
        hf = .5f;
        break;
      case rumble_type::kMedium:
        time_ticks = 12;
        hf = .5f;
        lf = .25f;
        break;
      case rumble_type::kLarge:
        time_ticks = 16;
        lf = .5f;
        hf = .25f;
        break;
      }
      for (std::uint32_t i = 0; i < hits.size(); ++i) {
        if (tick - hits[i].tick <= kRecentHitMaxTicks) {
          e_destroy.rumble(i, time_ticks, lf, hf);
        }
      }
    }
    if (on_destroy) {
      vec2 average_source = source_v;
      std::uint32_t count = 1;
      for (const auto& hit : hits) {
        if (tick - hit.tick <= kRecentHitMaxTicks) {
          average_source += hit.source;
          ++count;
        }
      }
      on_destroy(h, sim, e_destroy, type, average_source / count);
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
  score += s * multiplier;
  ++multiplier_count;
  if (multiplier_count >= (1u << std::min(multiplier + 3, 23u))) {
    multiplier_count = 0;
    ++multiplier;
  }
}

}  // namespace ii
