#include "game/logic/sim/sim_state.h"
#include "game/logic/boss.h"
#include "game/logic/boss/chaser.h"
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
  Boss::fireworks_.clear();
  Boss::warnings_.clear();
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
    auto* p = interface_->add_new_ship<Player>(v, i);
    internals_->player_list.push_back(p);
  }
  overmind_ = std::make_unique<Overmind>(*interface_);

  internals_->index.on_component_add<Destroy>(
      [internals = internals_.get()](ecs::handle handle, const Destroy&) {
        if (auto c = handle.get<LegacyShip>(); c) {
          if (auto it = std::ranges::find(internals->collisions, c->ship.get());
              it != internals->collisions.end()) {
            internals->collisions.erase(it);
          }
        }
      });
}

std::uint32_t SimState::frame_count() const {
  return internals_->conditions.mode == game_mode::kFast ? 2 : 1;
}

void SimState::update() {
  Boss::warnings_.clear();
  colour_cycle_ = internals_->conditions.mode == game_mode::kHard ? 128
      : internals_->conditions.mode == game_mode::kFast           ? 192
      : internals_->conditions.mode == game_mode::kWhat           ? (colour_cycle_ + 1) % 256
                                                                  : 0;
  internals_->input_frames = input_.get();

  Player::update_fire_timer();
  ChaserBoss::has_counted_ = false;
  auto sort_ships = [](const Ship* a, const Ship* b) {
    return a->shape().centre.x - a->bounding_width() < b->shape().centre.x - b->bounding_width();
  };
  std::stable_sort(internals_->collisions.begin(), internals_->collisions.end(), sort_ships);
  internals_->non_wall_enemy_count = interface_->count_ships(Ship::kShipEnemy, Ship::kShipWall);

  internals_->index.iterate<LegacyShip>([](ecs::handle handle, auto& c) {
    if (!handle.has<Destroy>()) {
      c.ship->update();
    }
  });
  for (auto& particle : internals_->particles) {
    if (!particle.destroy) {
      particle.position += particle.velocity;
      particle.destroy = !--particle.timer;
    }
  }
  for (auto it = Boss::fireworks_.begin(); it != Boss::fireworks_.end();) {
    if (it->first > 0) {
      --(it++)->first;
      continue;
    }
    vec2 v = internals_->player_list[0]->shape().centre;
    internals_->player_list[0]->shape().centre = it->second.first;
    internals_->player_list[0]->explosion(glm::vec4{1.f});
    internals_->player_list[0]->explosion(it->second.second, 16);
    internals_->player_list[0]->explosion(glm::vec4{1.f}, 24);
    internals_->player_list[0]->explosion(it->second.second, 32);
    internals_->player_list[0]->shape().centre = v;
    it = Boss::fireworks_.erase(it);
  }

  bool compact = false;
  internals_->index.iterate<Destroy>([&](ecs::handle handle, const auto&) {
    compact = true;
    internals_->index.destroy(handle.id());
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
  internals_->index.iterate<LegacyShip>([&](const auto& c) {
    if (!(c.ship->type() & Ship::kShipPlayer)) {
      c.ship->render();
    }
  });
  for (const auto& ship : internals_->player_list) {
    ship->render();
  }

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
    if (c.ship->type() & Ship::kShipEnemy) {
      render_warning(to_float(c.ship->shape().centre));
    }
  });
  for (const auto& v : Boss::warnings_) {
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
  result.boss_hp_bar = internals_->boss_hp_bar;
  result.colour_cycle = colour_cycle_;
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

  for (auto* ship : interface_->players()) {
    auto* p = static_cast<Player*>(ship);
    auto& pr = r.players.emplace_back();
    pr.number = p->player_number();
    pr.score = p->score();
    pr.deaths = p->deaths();
  }
  return r;
}

}  // namespace ii