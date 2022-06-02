#include "game/logic/sim/sim_state.h"
#include "game/logic/boss.h"
#include "game/logic/boss/chaser.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/sim/sim_internals.h"
#include "game/logic/stars.h"
#include <algorithm>

namespace ii {

SimState::~SimState() {
  Stars::clear();
  Boss::fireworks_.clear();
  Boss::warnings_.clear();
}

SimState::SimState(const initial_conditions& conditions, InputAdapter& input)
: conditions_{conditions}
, input_{input}
, internals_{std::make_unique<SimInternals>(conditions_.seed)}
, interface_{std::make_unique<SimInterface>(internals_.get())} {
  static constexpr std::int32_t kStartingLives = 2;
  static constexpr std::int32_t kBossModeLives = 1;

  internals_->mode = conditions_.mode;
  internals_->lives = conditions_.mode == game_mode::kBoss
      ? conditions_.player_count * kBossModeLives
      : kStartingLives;

  Stars::clear();
  for (std::int32_t i = 0; i < conditions_.player_count; ++i) {
    vec2 v((1 + i) * kSimWidth / (1 + conditions_.player_count), kSimHeight / 2);
    auto* p = interface_->add_new_ship<Player>(v, i);
    internals_->player_list.push_back(p);
  }
  overmind_ = std::make_unique<Overmind>(*interface_, conditions_.can_face_secret_boss);
  internals_->overmind = overmind_.get();
}

std::int32_t SimState::frame_count() const {
  return conditions_.mode == game_mode::kFast ? 2 : 1;
}

void SimState::update() {
  Boss::warnings_.clear();
  colour_cycle_ = conditions_.mode == game_mode::kHard ? 128
      : conditions_.mode == game_mode::kFast           ? 192
      : conditions_.mode == game_mode::kWhat           ? (colour_cycle_ + 1) % 256
                                                       : 0;
  internals_->input_frames = input_.get();

  Player::update_fire_timer();
  ChaserBoss::has_counted_ = false;
  auto sort_ships = [](const Ship* a, const Ship* b) {
    return a->shape().centre.x - a->bounding_width() < b->shape().centre.x - b->bounding_width();
  };
  std::stable_sort(internals_->collisions.begin(), internals_->collisions.end(), sort_ships);
  for (std::size_t i = 0; i < internals_->ships.size(); ++i) {
    if (!internals_->ships[i]->is_destroyed()) {
      internals_->ships[i]->update();
    }
  }
  for (auto& particle : internals_->particles) {
    if (!particle.destroy) {
      particle.position += particle.velocity;
      if (--particle.timer <= 0) {
        particle.destroy = true;
      }
    }
  }
  for (auto it = Boss::fireworks_.begin(); it != Boss::fireworks_.end();) {
    if (it->first > 0) {
      --(it++)->first;
      continue;
    }
    vec2 v = internals_->ships[0]->shape().centre;
    internals_->ships[0]->shape().centre = it->second.first;
    internals_->ships[0]->explosion(0xffffffff);
    internals_->ships[0]->explosion(it->second.second, 16);
    internals_->ships[0]->explosion(0xffffffff, 24);
    internals_->ships[0]->explosion(it->second.second, 32);
    internals_->ships[0]->shape().centre = v;
    it = Boss::fireworks_.erase(it);
  }

  for (auto it = internals_->ships.begin(); it != internals_->ships.end();) {
    if (!(*it)->is_destroyed()) {
      ++it;
      continue;
    }

    if ((*it)->type() & Ship::kShipEnemy) {
      overmind_->on_enemy_destroy(**it);
    }
    for (auto jt = internals_->collisions.begin(); jt != internals_->collisions.end();) {
      if (it->get() == *jt) {
        jt = internals_->collisions.erase(jt);
        continue;
      }
      ++jt;
    }

    it = internals_->ships.erase(it);
  }

  for (auto it = internals_->particles.begin(); it != internals_->particles.end();) {
    if (it->destroy) {
      it = internals_->particles.erase(it);
      continue;
    }
    ++it;
  }
  overmind_->update();

  if (!kill_timer_ &&
      ((interface_->killed_players() == static_cast<std::int32_t>(interface_->players().size()) &&
        !interface_->get_lives()) ||
       (conditions_.mode == game_mode::kBoss && overmind_->get_killed_bosses() >= 6))) {
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
    interface_->render_line_rect(particle.position + fvec2{1, 1}, particle.position - fvec2{1, 1},
                                 particle.colour);
  }
  for (std::size_t i = internals_->player_list.size(); i < internals_->ships.size(); ++i) {
    internals_->ships[i]->render();
  }
  for (const auto& ship : internals_->player_list) {
    ship->render();
  }

  for (std::size_t i = 0; i < internals_->ships.size() + Boss::warnings_.size(); ++i) {
    if (i < internals_->ships.size() && !(internals_->ships[i]->type() & Ship::kShipEnemy)) {
      continue;
    }
    fvec2 v =
        to_float(i < internals_->ships.size() ? internals_->ships[i]->shape().centre
                                              : Boss::warnings_[i - internals_->ships.size()]);

    if (v.x < -4) {
      auto a = static_cast<std::int32_t>(.5f + float{0x1} +
                                         float{0x9} * std::max(v.x + kSimWidth, 0.f) / kSimWidth);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      interface_->render_line(fvec2{0.f, v.y}, fvec2{6, v.y - 3}, a);
      interface_->render_line(fvec2{6.f, v.y - 3}, fvec2{6, v.y + 3}, a);
      interface_->render_line(fvec2{6.f, v.y + 3}, fvec2{0, v.y}, a);
    }
    if (v.x >= kSimWidth + 4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(2 * kSimWidth - v.x, 0.f) / kSimWidth);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      interface_->render_line(fvec2{float{kSimWidth}, v.y}, fvec2{kSimWidth - 6.f, v.y - 3}, a);
      interface_->render_line(fvec2{kSimWidth - 6, v.y - 3}, fvec2{kSimWidth - 6.f, v.y + 3}, a);
      interface_->render_line(fvec2{kSimWidth - 6, v.y + 3}, fvec2{float{kSimWidth}, v.y}, a);
    }
    if (v.y < -4) {
      auto a = static_cast<std::int32_t>(.5f + float{0x1} +
                                         float{0x9} * std::max(v.y + kSimHeight, 0.f) / kSimHeight);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      interface_->render_line(fvec2{v.x, 0.f}, fvec2{v.x - 3, 6.f}, a);
      interface_->render_line(fvec2{v.x - 3, 6.f}, fvec2{v.x + 3, 6.f}, a);
      interface_->render_line(fvec2{v.x + 3, 6.f}, fvec2{v.x, 0.f}, a);
    }
    if (v.y >= kSimHeight + 4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(2 * kSimHeight - v.y, 0.f) / kSimHeight);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      interface_->render_line(fvec2{v.x, float{kSimHeight}}, fvec2{v.x - 3, kSimHeight - 6.f}, a);
      interface_->render_line(fvec2{v.x - 3, kSimHeight - 6.f}, fvec2{v.x + 3, kSimHeight - 6.f},
                              a);
      interface_->render_line(fvec2{v.x + 3, kSimHeight - 6.f}, fvec2{v.x, float{kSimHeight}}, a);
    }
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
  return result;
}

std::unordered_map<std::int32_t, std::int32_t> SimState::get_rumble_output() const {
  return internals_->rumble_output;
}

render_output SimState::get_render_output() const {
  render_output result;
  for (const auto& p : internals_->player_output) {
    auto& o = result.players.emplace_back();
    o.colour = p.colour;
    o.multiplier = p.multiplier;
    o.score = p.score;
    o.timer = p.timer;
  }
  for (const auto& line : internals_->line_output) {
    auto& l = result.lines.emplace_back();
    l.a = line.a;
    l.b = line.b;
    l.c = line.c;
  }
  result.mode = conditions_.mode;
  result.elapsed_time = overmind_->get_elapsed_time();
  result.lives_remaining = interface_->get_lives();
  result.overmind_timer = overmind_->get_timer();
  result.boss_hp_bar = internals_->boss_hp_bar;
  result.colour_cycle = colour_cycle_;
  return result;
}

sim_results SimState::get_results() const {
  sim_results r;
  r.mode = conditions_.mode;
  r.seed = conditions_.seed;
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