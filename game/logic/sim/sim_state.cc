#include "game/logic/sim/sim_state.h"
#include "game/logic/ecs/call.h"
#include "game/logic/overmind/overmind.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_internals.h"
#include <algorithm>
#include <unordered_set>

namespace ii {

SimState::~SimState() = default;

SimState::SimState(const initial_conditions& conditions, InputAdapter& input,
                   std::span<const std::uint32_t> ai_players)
: input_{input}
, internals_{std::make_unique<SimInternals>(conditions.seed)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  internals_->conditions = conditions;
  internals_->global_entity_handle = internals_->index.create(GlobalData{conditions});
  internals_->global_entity_handle->add(Update{.update = ecs::call<&GlobalData::pre_update>});
  internals_->global_entity_handle->add(
      PostUpdate{.post_update = ecs::call<&GlobalData::post_update>});
  internals_->global_entity_id = internals_->global_entity_handle->id();

  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v((1 + i) * kSimDimensions.x / (1 + conditions.player_count), kSimDimensions.y / 2);
    spawn_player(*interface_, v, i, /* AI */ std::ranges::find(ai_players, i) != ai_players.end());
  }
  overmind_ = std::make_unique<Overmind>(*interface_);

  auto* internals = internals_.get();
  auto* interface = interface_.get();
  internals_->index.on_component_add<Collision>([internals](ecs::handle h, const Collision& c) {
    internals->collisions.emplace_back(
        SimInternals::collision_entry{h.id(), h, h.get<Transform>(), &c, 0});
  });
  internals_->index.on_component_add<Destroy>(
      [internals, interface](ecs::handle h, const Destroy& d) {
        auto* e = h.get<Enemy>();
        if (e && e->score_reward && d.source) {
          if (auto* p = internals->index.get<Player>(*d.source); p) {
            p->add_score(*interface, e->score_reward);
          }
        }
        if (e && e->boss_score_reward) {
          internals->index.iterate<Player>([&](Player& p) {
            if (!p.is_killed()) {
              p.add_score(*interface, e->boss_score_reward / interface->alive_players());
            }
          });
        }
        if (auto* b = h.get<Boss>(); b) {
          if (b->boss == boss_flag::kBoss3A ||
              (internals->conditions.mode != game_mode::kBoss &&
               internals->conditions.mode != game_mode::kNormal)) {
            internals->hard_mode_bosses_killed |= b->boss;
          } else {
            internals->bosses_killed |= b->boss;
          }
        }

        if (auto it = std::ranges::find(internals->collisions, h.id(),
                                        [](const auto& e) { return e.handle.id(); });
            it != internals->collisions.end()) {
          internals->collisions.erase(it);
        }
      });
}

std::uint32_t SimState::frame_count() const {
  return internals_->conditions.mode == game_mode::kFast ? 2 : 1;
}

void SimState::update() {
  colour_cycle_ = internals_->conditions.mode == game_mode::kHard ? 128
      : internals_->conditions.mode == game_mode::kFast           ? 192
      : internals_->conditions.mode == game_mode::kWhat           ? (colour_cycle_ + 1) % 256
                                                                  : 0;
  internals_->input_frames = &input_.get();

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
  for (auto& particle : internals_->particles) {
    if (!particle.destroy) {
      particle.position += particle.velocity;
      particle.destroy = !--particle.timer;
    }
  }

  internals_->index.iterate_dispatch<Destroy>([&](ecs::const_handle h) {
    internals_->index.destroy(h.id());
    ++compact_counter_;
  });

  if (compact_counter_ >= internals_->index.size()) {
    internals_->index.compact();
    for (auto& e : internals_->collisions) {
      e.handle = *internals_->index.get(e.id);
      e.collision = e.handle.get<Collision>();
      e.transform = e.handle.get<Transform>();
    }
    internals_->global_entity_handle = internals_->index.get(internals_->global_entity_id);
    compact_counter_ = 0;
  }

  std::erase_if(internals_->particles, [](const particle& p) { return p.destroy; });
  internals_->index.iterate_dispatch<PostUpdate>([&](ecs::handle h, PostUpdate& c) {
    if (!h.has<Destroy>()) {
      c.post_update(h, *interface_);
    }
  });
  overmind_->update();

  if (!kill_timer_ &&
      ((interface_->killed_players() == interface_->player_count() && !interface_->get_lives()) ||
       (internals_->conditions.mode == game_mode::kBoss && overmind_->get_killed_bosses() >= 6))) {
    kill_timer_ = 100;
  }
  if (kill_timer_) {
    kill_timer_--;
    if (!kill_timer_) {
      game_over_ = true;
    }
  }
  ++internals_->tick_count;
  input_.next();
}

void SimState::render() const {
  internals_->boss_hp_bar.reset();
  internals_->line_output.clear();
  internals_->player_output.clear();

  overmind_->render();
  for (const auto& particle : internals_->particles) {
    interface_->render_line_rect(particle.position + glm::vec2{1, 1},
                                 particle.position - glm::vec2{1, 1}, particle.colour);
  }
  internals_->index.iterate_dispatch<Render>([&](ecs::const_handle h, const Render& r) {
    if (!h.get<Player>()) {
      r.render(h, *interface_);
    }
  });
  internals_->index.iterate_dispatch<Player>(
      [&](ecs::const_handle h, const Render& r) { r.render(h, *interface_); });

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
}

bool SimState::game_over() const {
  return game_over_;
}

void SimState::clear_output() {
  internals_->sound_output.clear();
  internals_->rumble_output.clear();
}

std::unordered_map<sound, sound_out> SimState::get_sound_output() const {
  std::unordered_map<sound, sound_out> result;
  for (const auto& pair : internals_->sound_output) {
    sound_out s;
    s.volume = std::max(0.f, std::min(1.f, pair.second.volume));
    s.pan = pair.second.pan / static_cast<float>(pair.second.count);
    s.pitch = std::pow(2.f, pair.second.pitch);
    result.emplace(pair.first, s);
  }
  internals_->sound_output.clear();
  return result;
}

std::unordered_map<std::uint32_t, std::uint32_t> SimState::get_rumble_output() const {
  return internals_->rumble_output;
}

render_output SimState::get_render_output() const {
  render_output result;
  result.players = internals_->player_output;
  result.lines = std::move(internals_->line_output);
  result.mode = internals_->conditions.mode;
  result.tick_count = internals_->tick_count;
  result.lives_remaining = interface_->get_lives();
  result.overmind_timer = overmind_->get_timer();
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

sim_results SimState::get_results() const {
  sim_results r;
  r.mode = internals_->conditions.mode;
  r.seed = internals_->conditions.seed;
  r.tick_count = internals_->tick_count;
  r.killed_bosses = overmind_->get_killed_bosses();
  r.lives_remaining = interface_->get_lives();
  r.bosses_killed = internals_->bosses_killed;
  r.hard_mode_bosses_killed = internals_->hard_mode_bosses_killed;

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

}  // namespace ii