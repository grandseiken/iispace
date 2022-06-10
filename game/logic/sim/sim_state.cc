#include "game/logic/sim/sim_state.h"
#include "game/logic/boss.h"
#include "game/logic/enemy.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship/ship.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_internals.h"
#include "game/logic/stars.h"
#include <algorithm>
#include <unordered_set>

namespace ii {

SimState::~SimState() {
  Stars::clear();
  boss_fireworks().clear();
  boss_warnings().clear();
}

SimState::SimState(const initial_conditions& conditions, InputAdapter& input)
: input_{input}
, internals_{std::make_unique<SimInternals>(conditions.seed)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  static constexpr std::uint32_t kStartingLives = 2;
  static constexpr std::uint32_t kBossModeLives = 1;

  internals_->conditions = conditions;
  internals_->lives = conditions.mode == game_mode::kBoss ? conditions.player_count * kBossModeLives
                                                          : kStartingLives;

  Stars::clear();
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    vec2 v((1 + i) * kSimDimensions.x / (1 + conditions.player_count), kSimDimensions.y / 2);
    spawn_player(*interface_, v, i);
  }
  overmind_ = std::make_unique<Overmind>(*interface_);

  auto internals = internals_.get();
  auto interface = interface_.get();
  internals_->index.on_component_add<Collision>([internals](ecs::handle h, const Collision& c) {
    internals->collisions.emplace_back(SimInternals::collision_entry{h, 0, c.bounding_width});
  });
  internals_->index.on_component_add<Destroy>(
      [internals, interface](ecs::handle h, const Destroy& d) {
        auto e = h.get<Enemy>();
        if (e && e->score_reward && d.source) {
          if (auto* p = internals->index.get<Player>(*d.source); p) {
            p->add_score(*interface, e->score_reward);
          }
        }
        if (e && e->boss_score_reward) {
          internals->index.iterate<Player>([&](ecs::handle h, Player& p) {
            if (!p.is_killed()) {
              p.add_score(*interface, e->boss_score_reward / interface->alive_players());
            }
          });
        }
        if (auto b = h.get<Boss>(); b) {
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
  boss_warnings().clear();
  colour_cycle_ = internals_->conditions.mode == game_mode::kHard ? 128
      : internals_->conditions.mode == game_mode::kFast           ? 192
      : internals_->conditions.mode == game_mode::kWhat           ? (colour_cycle_ + 1) % 256
                                                                  : 0;
  internals_->input_frames = input_.get();

  ::Player::update_fire_timer();
  chaser_boss_begin_frame();

  for (auto& e : internals_->collisions) {
    auto& c = *e.handle.get<Collision>();
    e.x_min = c.centre(e.handle).x - c.bounding_width;
  }
  std::ranges::stable_sort(internals_->collisions,
                           [](const auto& a, const auto& b) { return a.x_min < b.x_min; });

  internals_->non_wall_enemy_count = interface_->count_ships(ship_flag::kEnemy, ship_flag::kWall);

  internals_->index.iterate<Boss>([&](ecs::const_handle h, Boss& boss) {
    std::optional<vec2> position;
    if (auto* c = h.get<Transform>(); c) {
      position = c->centre;
    } else if (auto* c = h.get<LegacyShip>(); c) {
      position = c->ship->position();
    }
    if (position && interface_->is_on_screen(*position)) {
      boss.show_hp_bar = true;
    }
  });
  internals_->index.iterate<Health>([](Health& h) {
    if (h.hit_timer) {
      --h.hit_timer;
    }
  });
  internals_->index.iterate<Update>([&](ecs::handle h, Update& c) {
    if (!h.has<Destroy>()) {
      c.update(*interface_, h);
    }
  });
  for (auto& particle : internals_->particles) {
    if (!particle.destroy) {
      particle.position += particle.velocity;
      particle.destroy = !--particle.timer;
    }
  }
  for (auto it = boss_fireworks().begin(); it != boss_fireworks().end();) {
    if (it->first > 0) {
      --(it++)->first;
      continue;
    }
    auto* p = static_cast<::Player*>(interface_->players().front());
    vec2 v = p->shape().centre;
    p->shape().centre = it->second.first;
    p->explosion(glm::vec4{1.f});
    p->explosion(it->second.second, 16);
    p->explosion(glm::vec4{1.f}, 24);
    p->explosion(it->second.second, 32);
    p->shape().centre = v;
    it = boss_fireworks().erase(it);
  }

  bool compact = false;
  internals_->index.iterate<Destroy>([&](ecs::handle h, const Destroy&) {
    compact = true;
    internals_->index.destroy(h.id());
  });

  if (compact) {
    internals_->index.compact();
  }

  internals_->particles.erase(
      std::remove_if(internals_->particles.begin(), internals_->particles.end(),
                     [](const particle& p) { return p.destroy; }),
      internals_->particles.end());
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
}

void SimState::render() const {
  internals_->boss_hp_bar.reset();
  internals_->line_output.clear();
  internals_->player_output.clear();

  Stars::render(*interface_);
  for (const auto& particle : internals_->particles) {
    interface_->render_line_rect(particle.position + glm::vec2{1, 1},
                                 particle.position - glm::vec2{1, 1}, particle.colour);
  }
  internals_->index.iterate<Render>([&](ecs::const_handle h, const auto& c) {
    if (!h.get<Player>()) {
      c.render(*interface_, h);
    }
  });
  internals_->index.iterate<Player>(
      [&](ecs::const_handle h, const auto&) { h.get<Render>()->render(*interface_, h); });

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

  internals_->index.iterate<LegacyShip>([&render_warning](const auto& c) {
    if (+(c.ship->type() & ship_flag::kEnemy)) {
      render_warning(to_float(c.ship->position()));
    }
  });
  for (const auto& v : boss_warnings()) {
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
    s.pan = pair.second.pan / pair.second.count;
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
  result.elapsed_time = overmind_->get_elapsed_time();
  result.lives_remaining = interface_->get_lives();
  result.overmind_timer = overmind_->get_timer();
  result.colour_cycle = colour_cycle_;

  std::uint32_t boss_hp = 0;
  std::uint32_t boss_max_hp = 0;
  internals_->index.iterate<Boss>([&](ecs::const_handle h, const Boss& boss) {
    if (auto c = h.get<Health>(); c && boss.show_hp_bar) {
      boss_hp += c->hp;
      boss_max_hp += c->max_hp;
    }
  });
  if (boss_hp && boss_max_hp) {
    result.boss_hp_bar = static_cast<float>(boss_hp) / boss_max_hp;
  }
  return result;
}

sim_results SimState::get_results() const {
  sim_results r;
  r.mode = internals_->conditions.mode;
  r.seed = internals_->conditions.seed;
  r.elapsed_time = overmind_->get_elapsed_time();
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
  return r;
}

}  // namespace ii