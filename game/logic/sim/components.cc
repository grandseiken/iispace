#include "game/logic/sim/components.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

void Render::render_shapes(ecs::const_handle h, render::entity_state& state, bool paused,
                           std::vector<render::shape>& output, const SimInterface& sim) {
  // TODO: this is basically hard to control. clear_trails is a hack due to
  // not being able to specify which shape is which from frame to frame.
  // What we should do is having some way of outputting independent shape _groups_ per-entity.
  if (clear_trails) {
    state.trails.clear();
    clear_trails = false;
  }
  auto start = output.size();
  render(h, output, sim);
  auto count = output.size() - start;
  state.trails.resize(count);
  for (std::size_t i = 0; i < count; ++i) {
    auto& s = output[start + i];
    if (s.disable_trail) {
      s.trail.reset();
      state.trails[i].reset();
    } else if (state.trails[i]) {
      s.trail = *state.trails[i];
    }
    if (!paused) {
      state.trails[i] = render::motion_trail{
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

}  // namespace ii