#include "game/logic/sim/sim_state.h"
#include "game/data/replay.h"
#include "game/logic/ecs/call.h"
#include "game/logic/legacy/components.h"
#include "game/logic/legacy/setup.h"
#include "game/logic/sim/io/render.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_internals.h"
#include "game/logic/v0/ai/ai_player.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/setup.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <unordered_set>

namespace ii {
namespace {

std::unique_ptr<SimSetup> make_sim_setup(const initial_conditions& conditions) {
  if (conditions.mode == game_mode::kStandardRun) {
    return std::make_unique<v0::V0SimSetup>();
  }
  return std::make_unique<legacy::LegacySimSetup>();
}

void initialise_systems(SimInternals& internals) {
  // TODO: should collision be some kind of System implementation that can be auto-hooked up?
  internals.index.on_component_add<Collision>(
      [&internals](ecs::handle h, const Collision& c) { internals.collision_index->add(h, c); });
  internals.index.on_component_add<Destroy>(
      [&internals](ecs::handle h, const Destroy&) { internals.collision_index->remove(h); });
}

void refresh_handles(SimInternals& internals) {
  internals.collision_index->refresh_handles(internals.index);
  internals.global_entity_handle = internals.index.get(internals.global_entity_id);
}

}  // namespace

SimState::~SimState() = default;
SimState::SimState(SimState&&) noexcept = default;
SimState& SimState::operator=(SimState&&) noexcept = default;

SimState::SimState()
: internals_{std::make_unique<SimInternals>(/* seed */ 0)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  initialise_systems(*internals_);
}

SimState::SimState(const initial_conditions& conditions, data::ReplayWriter* replay_writer,
                   std::span<const std::uint32_t> ai_players)
: replay_writer_{replay_writer}
, internals_{std::make_unique<SimInternals>(conditions.seed)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  setup_ = make_sim_setup(conditions);
  internals_->conditions = conditions;
  internals_->dimensions = setup_->parameters(internals_->conditions).dimensions;

  if (conditions.compatibility == compatibility_level::kLegacy) {
    internals_->collision_index = std::make_unique<LegacyCollisionIndex>();
  } else {
    internals_->collision_index = std::make_unique<GridCollisionIndex>(
        glm::uvec2{64, 64}, glm::ivec2{-128, -128},
        glm::ivec2{internals_->dimensions.x.to_int(), internals_->dimensions.y.to_int()} +
            glm::ivec2{128, 128});
  }

  internals_->global_entity_id = setup_->start_game(conditions, *interface_);
  internals_->index.iterate_dispatch<Player>([&](ecs::handle h, const Player& p) {
    if (std::find(ai_players.begin(), ai_players.end(), p.player_number) != ai_players.end()) {
      v0::add_ai(h);
    }
  });
  setup_->initialise_systems(*interface_);
  initialise_systems(*internals_);
  refresh_handles(*internals_);
}

glm::uvec2 SimState::dimensions() const {
  return {static_cast<std::uint32_t>(interface_->dimensions().x.to_int()),
          static_cast<std::uint32_t>(interface_->dimensions().y.to_int())};
}

std::uint64_t SimState::tick_count() const {
  return internals_->tick_count;
}

std::uint32_t SimState::checksum() const {
  std::size_t result = 0;
  internals_->index.iterate_dispatch<Transform>(
      [&](ecs::const_handle h, const Transform& transform) {
        hash_combine(result, +h.id());
        hash_combine(result, static_cast<std::size_t>(transform.centre.x.to_internal()));
        hash_combine(result, static_cast<std::size_t>(transform.centre.y.to_internal()));
      });
  return static_cast<std::uint32_t>(result);
}

void SimState::copy_to(SimState& target) const {
  if (&target == this) {
    return;
  }
  if (!target.setup_) {
    target.setup_ = make_sim_setup(internals_->conditions);
    target.setup_->initialise_systems(*target.interface_);
  }
  target.close_timer_ = close_timer_;
  target.colour_cycle_ = colour_cycle_;
  target.game_over_ = game_over_;
  target.compact_counter_ = 0;

  internals_->index.copy_to(target.internals_->index);
  target.internals_->input_frames.clear();
  target.internals_->game_state_random.set_state(internals_->game_state_random.state());
  target.internals_->game_sequence_random.set_state(internals_->game_sequence_random.state());
  target.internals_->aesthetic_random.set_state(internals_->aesthetic_random.state());
  target.internals_->conditions = internals_->conditions;
  target.internals_->dimensions = internals_->dimensions;
  target.internals_->global_entity_id = internals_->global_entity_id;
  target.internals_->global_entity_handle.reset();
  target.internals_->tick_count = internals_->tick_count;
  target.internals_->collision_index = internals_->collision_index->clone();
  target.internals_->results = internals_->results;
  target.internals_->output.clear();
  refresh_handles(*target.internals_);
}

void SimState::ai_think(std::vector<input_frame>& input) const {
  ai_think(input, ai_state_);
}

void SimState::ai_think(std::vector<input_frame>& input, std::vector<ai_state>& state) const {
  input.resize(internals_->conditions.player_count);
  state.resize(internals_->conditions.player_count);
  internals_->index.iterate_dispatch<Player>([&](ecs::handle h, const Player& p) {
    if (auto f = v0::ai_think(*interface_, h, state[p.player_number]); f) {
      input[p.player_number] = *f;
    }
  });
}

void SimState::update(std::vector<input_frame> input) {
  colour_cycle_ = internals_->conditions.mode == game_mode::kLegacy_Hard ? 128
      : internals_->conditions.mode == game_mode::kLegacy_Fast           ? 192
      : internals_->conditions.mode == game_mode::kLegacy_What           ? (colour_cycle_ + 3) % 256
                                                                         : 0;
  internals_->input_frames = std::move(input);
  internals_->input_frames.resize(internals_->conditions.player_count);
  internals_->collision_index->begin_tick();
  setup_->begin_tick(*interface_);

  internals_->index.iterate_dispatch_if<Boss>([&](Boss& boss, Transform& transform) {
    if (interface_->is_on_screen(transform.centre)) {
      boss.show_hp_bar = true;
    }
  });
  internals_->index.iterate<Health>([](Health& h) { h.hit_timer && --h.hit_timer; });
  internals_->index.iterate_dispatch<Update>([&](ecs::handle h, Update& c) {
    if (!h.has<Destroy>()) {
      c.update(h, *interface_);
      // TODO: we only update after the entity itself has updated: this can still lead to minor
      // inconsistencies if the entity is moved by some external force.
      internals_->collision_index->update(h);
    }
  });

  internals_->index.iterate_dispatch<Destroy>([&](ecs::const_handle h) {
    internals_->index.destroy(h.id());
    ++compact_counter_;
  });

  if (compact_counter_ >= internals_->index.size()) {
    internals_->index.compact();
    refresh_handles(*internals_);
    compact_counter_ = 0;
  }

  internals_->index.iterate_dispatch<PostUpdate>([&](ecs::handle h, PostUpdate& c) {
    if (!h.has<Destroy>()) {
      c.post_update(h, *interface_);
    }
  });

  if (!close_timer_ &&
      ((interface_->killed_players() == interface_->player_count() && !interface_->get_lives()) ||
       (internals_->conditions.mode == game_mode::kLegacy_Boss &&
        internals_->results.boss_kill_count() >= 6))) {
    close_timer_ = 100;
  }
  bool already_game_over = game_over_;
  if (close_timer_) {
    close_timer_--;
    if (!close_timer_) {
      game_over_ = true;
    }
  }
  ++internals_->tick_count;
  if (replay_writer_ && !already_game_over) {
    for (const auto& f : internals_->input_frames) {
      replay_writer_->add_input_frame(f);
    }
  }
}

bool SimState::game_over() const {
  return game_over_;
}

std::uint32_t SimState::fps() const {
  return setup_->parameters(internals_->conditions).fps;
}

render_output& SimState::render(transient_render_state& state, bool paused) const {
  auto& result = internals_->render;
  result.boss_hp_bar.reset();
  result.shapes.clear();
  result.players.clear();

  internals_->index.iterate_dispatch<Render>([&](ecs::handle h, Render& r) {
    if (!h.get<Player>()) {
      r.render_shapes(h, state.entity_map[+h.id()], paused, result.shapes, *interface_);
      return;
    }
  });
  internals_->index.iterate_dispatch<Player>(
      [&](ecs::handle h, const Player& p, Render& r, Transform& transform) {
        if (auto info = p.render_info(h, *interface_)) {
          result.players.resize(
              std::max(result.players.size(), static_cast<std::size_t>(p.player_number + 1)));
          result.players[p.player_number] = *info;
        }

        auto it = smoothing_data_.players.find(p.player_number);
        if (it == smoothing_data_.players.end() || !it->second.position) {
          r.render_shapes(h, state.entity_map[+h.id()], paused, result.shapes, *interface_);
          return;
        }
        auto transform_copy = transform;
        transform.centre = *it->second.position;
        transform.rotation = it->second.rotation;
        r.render_shapes(h, state.entity_map[+h.id()], paused, result.shapes, *interface_);
        transform = transform_copy;
      });
  std::erase_if(state.entity_map, [&](const auto& pair) {
    return !internals_->index.contains(ecs::entity_id{pair.first});
  });

  // TODO: extract somewhere?
  render::ngon warning_ngon{.radius = 4.f, .sides = 3};
  auto render_warning = [&](const glm::vec2& v) {
    auto render_tri = [&](const glm::vec2& position, float r, float f) {
      result.shapes.emplace_back(render::shape{
          .origin = position,
          .rotation = r,
          .colour = {0.f, 0.f, .2f + .6f * f, .4f},
          .data = warning_ngon,
      });
    };

    auto d = to_float(interface_->dimensions());
    if (v.x < -4) {
      auto f = std::max(v.x + d.x, 0.f) / d.x;
      render_tri({4, v.y}, glm::pi<float>(), f);
    }
    if (v.x >= d.x + 4) {
      auto f = std::max(2 * d.x - v.x, 0.f) / d.x;
      render_tri({d.x - 4, v.y}, 0.f, f);
    }
    if (v.y < -4) {
      auto f = std::max(v.y + d.y, 0.f) / d.y;
      render_tri({v.x, 4}, 3.f * glm::pi<float>() / 2.f, f);
    }
    if (v.y >= d.y + 4) {
      auto f = std::max(2 * d.y - v.y, 0.f) / d.y;
      render_tri({v.x, d.y - 4}, glm::pi<float>() / 2.f, f);
    }
  };

  internals_->index.iterate_dispatch_if<Enemy>([&render_warning](const Transform& transform) {
    render_warning(to_float(transform.centre));
  });

  if (const auto* data = interface_->global_entity().get<legacy::GlobalData>(); data) {
    for (const auto& v : data->extra_enemy_warnings) {
      render_warning(to_float(v));
    }
    result.overmind_timer = data->overmind_wave_timer;
  }
  if (const auto* data = interface_->global_entity().get<v0::GlobalData>(); data) {
    result.overmind_wave = data->overmind_wave_count;
    result.debug_text = data->debug_text;
  }

  result.tick_count = internals_->tick_count;
  result.lives_remaining = interface_->get_lives();
  result.colour_cycle = colour_cycle_;

  std::uint32_t boss_hp = 0;
  std::uint32_t boss_max_hp = 0;
  internals_->index.iterate_dispatch_if<Boss>([&](const Boss& boss, const Health& health) {
    if (boss.show_hp_bar) {
      boss_hp += health.hp;
      boss_max_hp += health.max_hp;
    }
  });
  if (boss_hp && boss_max_hp) {
    result.boss_hp_bar = static_cast<float>(boss_hp) / static_cast<float>(boss_max_hp);
  }
  return result;
}

aggregate_output& SimState::output() {
  return internals_->output;
}

const sim_results& SimState::results() const {
  auto& r = internals_->results;
  r.seed = internals_->conditions.seed;
  r.tick_count = internals_->tick_count;
  r.lives_remaining = interface_->get_lives();

  r.players.clear();
  internals_->index.iterate_dispatch<Player>([&](ecs::handle h, const Player& p) {
    auto& pr = r.players.emplace_back();
    pr.number = p.player_number;
    pr.deaths = p.death_count;
    if (auto info = p.render_info(h, *interface_)) {
      pr.score = info->score;
    }
  });
  r.score = 0;
  if (internals_->conditions.mode == game_mode::kStandardRun ||
      internals_->conditions.mode == game_mode::kLegacy_Boss) {
    r.score = r.tick_count;
  } else {
    for (const auto& p : r.players) {
      r.score += p.score;
    }
  }
  return r;
}

void SimState::set_predicted_players(std::span<const std::uint32_t> player_ids) {
  internals_->index.iterate<Player>([&](Player& p) {
    p.is_predicted =
        std::find(player_ids.begin(), player_ids.end(), p.player_number) != player_ids.end();
  });
}

void SimState::update_smoothing(smoothing_data& data) {
  static constexpr fixed kMaxRotationSpeed = fixed_c::pi / 16;
  auto smooth_rotate = [&](fixed& x, fixed target) {
    auto d = angle_diff(x, target);
    if (abs(d) < kMaxRotationSpeed) {
      x = target;
    } else {
      x += std::clamp(d, -kMaxRotationSpeed, kMaxRotationSpeed);
    }
  };

  // TODO: possibly this should be some kind of smoothing component and made a bit more generic
  // so it can be applied to non-players as well.
  // TODO: detect when positions have actually diverged and only apply smoothing in that case
  // (i.e. add entries in a separate check_smoothing() function called on rollback/reapply and
  // remove when resolved)?
  internals_->index.iterate_dispatch<Player>([&](const Player& p, const Transform& transform) {
    auto it = data.players.find(p.player_number);
    if (it == data.players.end()) {
      return;
    }
    vec2 v{0, 0};
    fixed d{0};
    if (it->second.position) {
      v = transform.centre - *it->second.position;
      d = length(v);
    }
    if (!it->second.position || d < p.speed || p.is_killed) {
      it->second.velocity = {0, 0};
      if (p.is_killed) {
        // Avoid interpolating to spawn position after death.
        it->second.position.reset();
      } else {
        it->second.position = transform.centre;
      }
      smooth_rotate(it->second.rotation, transform.rotation);
      return;
    }
    auto target_speed = std::max(p.speed, std::min(p.speed * 9_fx / 8_fx, d / (2 * p.speed)));
    auto target_velocity = target_speed * normalise(v);
    it->second.velocity = rc_smooth(it->second.velocity, target_velocity, 3_fx / 4_fx);
    *it->second.position += it->second.velocity;
    smooth_rotate(it->second.rotation, angle(it->second.velocity));
  });
  smoothing_data_ = data;
}

void SimState::dump(Printer& printer, const query& q) const {
  printer.begin().put("game_state_random: ").put(internals_->game_state_random.state()).end();
  printer.begin().put("game_sequence_random: ").put(internals_->game_sequence_random.state()).end();
  printer.begin().put("aesthetic_random: ").put(internals_->aesthetic_random.state()).end().end();
  ecs::EntityIndex::query index_q;
  for (const auto& id : q.entity_ids) {
    index_q.entity_ids.emplace(ecs::entity_id{id});
  }
  for (const auto& name : q.component_names) {
    index_q.components.emplace(name);
  }
  printer.print_pointers = !q.portable;
  internals_->index.dump(printer, q.portable, index_q);
}

}  // namespace ii
