#include "game/logic/sim/sim_state.h"
#include "game/data/replay.h"
#include "game/logic/ecs/call.h"
#include "game/logic/overmind/overmind.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_internals.h"
#include <algorithm>
#include <unordered_set>

namespace ii {
namespace {

void setup_index_callbacks(SimInterface& interface, SimInternals& internals) {
  internals.index.on_component_add<Collision>([&internals](ecs::handle h, const Collision& c) {
    internals.collisions.emplace_back(
        SimInternals::collision_entry{h.id(), h, h.get<Transform>(), &c, 0});
  });
  internals.index.on_component_add<Destroy>(
      [&internals, &interface](ecs::handle h, const Destroy& d) {
        auto* e = h.get<Enemy>();
        if (e && e->score_reward && d.source) {
          if (auto* p = internals.index.get<Player>(*d.source); p) {
            p->add_score(interface, e->score_reward);
          }
        }
        if (e && e->boss_score_reward) {
          internals.index.iterate<Player>([&](Player& p) {
            if (!p.is_killed()) {
              p.add_score(interface, e->boss_score_reward / interface.alive_players());
            }
          });
        }
        if (auto* b = h.get<Boss>(); b) {
          internals.bosses_killed |= b->boss;
        }

        if (auto it = std::ranges::find(internals.collisions, h.id(),
                                        [](const auto& e) { return e.handle.id(); });
            it != internals.collisions.end()) {
          internals.collisions.erase(it);
        }
      });
}

void refresh_handles(SimInternals& internals) {
  for (auto& e : internals.collisions) {
    e.handle = *internals.index.get(e.id);
    e.collision = e.handle.get<Collision>();
    e.transform = e.handle.get<Transform>();
  }
  internals.global_entity_handle = internals.index.get(internals.global_entity_id);
}

}  // namespace

SimState::~SimState() = default;
SimState::SimState(SimState&&) noexcept = default;
SimState& SimState::operator=(SimState&&) noexcept = default;

SimState::SimState()
: internals_{std::make_unique<SimInternals>(/* seed */ 0)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  setup_index_callbacks(*interface_, *internals_);
}

SimState::SimState(const initial_conditions& conditions, ReplayWriter* replay_writer,
                   std::span<const std::uint32_t> ai_players)
: replay_writer_{replay_writer}
, internals_{std::make_unique<SimInternals>(conditions.seed)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  internals_->conditions = conditions;
  internals_->global_entity_handle = internals_->index.create(GlobalData{conditions});
  internals_->global_entity_handle->add(Update{.update = ecs::call<&GlobalData::pre_update>});
  internals_->global_entity_handle->add(
      PostUpdate{.post_update = ecs::call<&GlobalData::post_update>});
  internals_->global_entity_id = internals_->global_entity_handle->id();
  spawn_overmind(*interface_);

  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v((1 + i) * kSimDimensions.x / (1 + conditions.player_count), kSimDimensions.y / 2);
    spawn_player(*interface_, v, i,
                 /* AI */ std::ranges::find(ai_players, i) != ai_players.end());
  }
  setup_index_callbacks(*interface_, *internals_);
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
  target.kill_timer_ = kill_timer_;
  target.colour_cycle_ = colour_cycle_;
  target.game_over_ = game_over_;
  target.compact_counter_ = 0;

  internals_->index.copy_to(target.internals_->index);
  target.internals_->input_frames.clear();
  target.internals_->game_state_random = internals_->game_state_random;
  target.internals_->aesthetic_random = internals_->aesthetic_random;
  target.internals_->conditions = internals_->conditions;
  target.internals_->global_entity_id = internals_->global_entity_id;
  target.internals_->global_entity_handle.reset();
  target.internals_->tick_count = internals_->tick_count;
  target.internals_->stars = internals_->stars;
  target.internals_->collisions = internals_->collisions;
  target.internals_->bosses_killed = internals_->bosses_killed;
  target.internals_->output.clear();
  refresh_handles(*target.internals_);
}

void SimState::ai_think(std::vector<input_frame>& input) const {
  input.resize(internals_->conditions.player_count);
  internals_->index.iterate_dispatch<Player>([&](ecs::handle h, const Player& p) {
    if (auto f = ii::ai_think(*interface_, h); f) {
      input[p.player_number] = *f;
    }
  });
}

void SimState::update(std::vector<input_frame> input) {
  colour_cycle_ = internals_->conditions.mode == game_mode::kHard ? 128
      : internals_->conditions.mode == game_mode::kFast           ? 192
      : internals_->conditions.mode == game_mode::kWhat           ? (colour_cycle_ + 1) % 256
                                                                  : 0;
  internals_->input_frames = std::move(input);
  internals_->input_frames.resize(internals_->conditions.player_count);

  // TODO: write a better collision system for non-legacy-compatibility mode.
  for (auto& e : internals_->collisions) {
    e.x_min = e.transform->centre.x - e.collision->bounding_width;
  }
  std::ranges::stable_sort(internals_->collisions,
                           [](const auto& a, const auto& b) { return a.x_min < b.x_min; });

  internals_->index.iterate_dispatch_if<Boss>([&](Boss& boss, Transform& transform) {
    if (interface_->is_on_screen(transform.centre)) {
      boss.show_hp_bar = true;
    }
  });
  internals_->index.iterate<Health>([](Health& h) {
    if (h.hit_timer) {
      --h.hit_timer;
    }
  });
  internals_->index.iterate_dispatch<Update>([&](ecs::handle h, Update& c) {
    if (!h.has<Destroy>()) {
      c.update(h, *interface_);
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

  if (!kill_timer_ &&
      ((interface_->killed_players() == interface_->player_count() && !interface_->get_lives()) ||
       (internals_->conditions.mode == game_mode::kBoss &&
        boss_kill_count(internals_->bosses_killed) >= 6))) {
    kill_timer_ = 100;
  }
  bool already_game_over = game_over_;
  if (kill_timer_) {
    kill_timer_--;
    if (!kill_timer_) {
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

render_output SimState::render() const {
  internals_->boss_hp_bar.reset();
  internals_->line_output.clear();
  internals_->player_output.clear();

  internals_->stars.render(*interface_);
  internals_->index.iterate_dispatch<Render>([&](ecs::const_handle h, const Render& r) {
    if (!h.get<Player>()) {
      r.render(h, *interface_);
    }
  });
  internals_->index.iterate_dispatch<Player>(
      [&](ecs::handle h, const Player& p, const Render& r, Transform& transform) {
        auto it = smoothing_data_.players.find(p.player_number);
        if (it == smoothing_data_.players.end() || !it->second.position) {
          r.render(h, *interface_);
          return;
        }
        auto transform_copy = transform;
        transform.centre = *it->second.position;
        transform.rotation = it->second.rotation;
        r.render(h, *interface_);
        transform = transform_copy;
      });

  auto render_warning = [&](const glm::vec2& v) {
    if (v.x < -4) {
      auto f = .2f + .6f * std::max(v.x + kSimDimensions.x, 0.f) / kSimDimensions.x;
      glm::vec4 c{0.f, 0.f, f, .4f};
      interface_->render_line({0.f, v.y}, {6, v.y - 3}, c);
      interface_->render_line({6.f, v.y - 3}, {6, v.y + 3}, c);
      interface_->render_line({6.f, v.y + 3}, {0, v.y}, c);
    }
    if (v.x >= kSimDimensions.x + 4) {
      auto f = .2f + .6f * std::max(2 * kSimDimensions.x - v.x, 0.f) / kSimDimensions.x;
      glm::vec4 c{0.f, 0.f, f, .4f};
      interface_->render_line({float{kSimDimensions.x}, v.y}, {kSimDimensions.x - 6.f, v.y - 3}, c);
      interface_->render_line({kSimDimensions.x - 6, v.y - 3}, {kSimDimensions.x - 6.f, v.y + 3},
                              c);
      interface_->render_line({kSimDimensions.x - 6, v.y + 3}, {float{kSimDimensions.x}, v.y}, c);
    }
    if (v.y < -4) {
      auto f = .2f + .6f * std::max(v.y + kSimDimensions.y, 0.f) / kSimDimensions.y;
      glm::vec4 c{0.f, 0.f, f, .4f};
      interface_->render_line({v.x, 0.f}, {v.x - 3, 6.f}, c);
      interface_->render_line({v.x - 3, 6.f}, {v.x + 3, 6.f}, c);
      interface_->render_line({v.x + 3, 6.f}, {v.x, 0.f}, c);
    }
    if (v.y >= kSimDimensions.y + 4) {
      auto f = .2f + .6f * std::max(2 * kSimDimensions.y - v.y, 0.f) / kSimDimensions.y;
      glm::vec4 c{0.f, 0.f, f, .4f};
      interface_->render_line({v.x, float{kSimDimensions.y}}, {v.x - 3, kSimDimensions.y - 6.f}, c);
      interface_->render_line({v.x - 3, kSimDimensions.y - 6.f}, {v.x + 3, kSimDimensions.y - 6.f},
                              c);
      interface_->render_line({v.x + 3, kSimDimensions.y - 6.f}, {v.x, float{kSimDimensions.y}}, c);
    }
  };

  internals_->index.iterate_dispatch_if<Enemy>([&render_warning](const Transform& transform) {
    render_warning(to_float(transform.centre));
  });
  for (const auto& v : interface_->global_entity().get<GlobalData>()->extra_enemy_warnings) {
    render_warning(to_float(v));
  }

  render_output result;
  result.players = internals_->player_output;
  result.lines = std::move(internals_->line_output);
  result.mode = internals_->conditions.mode;
  result.tick_count = internals_->tick_count;
  result.lives_remaining = interface_->get_lives();
  result.overmind_timer = interface_->global_entity().get<GlobalData>()->overmind_wave_timer;
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

std::uint32_t SimState::frame_count() const {
  return internals_->conditions.mode == game_mode::kFast ? 2 : 1;
}

aggregate_output& SimState::output() {
  return internals_->output;
}

sim_results SimState::results() const {
  sim_results r;
  r.mode = internals_->conditions.mode;
  r.seed = internals_->conditions.seed;
  r.tick_count = internals_->tick_count;
  r.lives_remaining = interface_->get_lives();
  r.bosses_killed = internals_->bosses_killed;

  internals_->index.iterate<Player>([&](const Player& p) {
    auto& pr = r.players.emplace_back();
    pr.number = p.player_number;
    pr.score = p.score;
    pr.deaths = p.death_count;
  });
  if (r.mode == game_mode::kBoss) {
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
    p.is_predicted = std::ranges::find(player_ids, p.player_number) != player_ids.end();
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
    auto v = transform.centre - *it->second.position;
    auto distance = length(v);
    if (!it->second.position || length(v) < kPlayerSpeed) {
      it->second.velocity = {0, 0};
      it->second.position = transform.centre;
      smooth_rotate(it->second.rotation, transform.rotation);
      return;
    }
    auto target_speed =
        std::max(kPlayerSpeed, std::min(kPlayerSpeed * 9_fx / 8_fx, distance / (2 * kPlayerSpeed)));
    auto target_velocity = target_speed * normalise(v);
    it->second.velocity = (3_fx / 4_fx) * (it->second.velocity - target_velocity) + target_velocity;
    *it->second.position += it->second.velocity;
    smooth_rotate(it->second.rotation, angle(it->second.velocity));
  });
  smoothing_data_ = data;
}

void SimState::dump(Printer& printer, const query& q) const {
  ecs::EntityIndex::query index_q;
  for (const auto& id : q.entity_ids) {
    index_q.entity_ids.emplace(ecs::entity_id{id});
  }
  for (const auto& name : q.component_names) {
    index_q.components.emplace(name);
  }
  internals_->index.dump(printer, index_q);
}

}  // namespace ii