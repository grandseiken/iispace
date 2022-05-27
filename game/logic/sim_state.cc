#include "game/logic/sim_state.h"
#include "game/core/save.h"
#include "game/io/file/filesystem.h"
#include "game/logic/boss.h"
#include "game/logic/boss/chaser.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/player_input.h"
#include "game/logic/ship.h"
#include "game/logic/stars.h"

SimState::SimState(Lib& lib, SaveData& save, std::int32_t* frame_count, game_mode mode,
                   std::int32_t player_count, bool can_face_secret_boss)
: SimState{lib, save, frame_count, Replay{mode, player_count, can_face_secret_boss}, true} {}

SimState::SimState(Lib& lib, SaveData& save, std::int32_t* frame_count,
                   const std::string& replay_path)
: SimState{lib, save, frame_count, Replay{lib.filesystem(), replay_path}, false} {
  lib.set_player_count(replay_.replay.players());
  lib.new_game();
}

SimState::~SimState() {
  Stars::clear();
  Boss::fireworks_.clear();
  Boss::warnings_.clear();
  *frame_count_ = 1;
}

void SimState::update(Lib& lib) {
  Boss::warnings_.clear();
  lib.set_colour_cycle(mode() == game_mode::kHard       ? 128
                           : mode() == game_mode::kFast ? 192
                           : mode() == game_mode::kWhat ? (lib.get_colour_cycle() + 1) % 256
                                                        : 0);
  if (replay_recording_) {
    *frame_count_ = mode() == game_mode::kFast ? 2 : 1;
  }

  if (!replay_recording_) {
    if (lib.is_key_pressed(Lib::key::kBomb)) {
      *frame_count_ *= 2;
    }
    if (lib.is_key_pressed(Lib::key::kFire) &&
        *frame_count_ > (mode() == game_mode::kFast ? 2 : 1)) {
      *frame_count_ /= 2;
    }
  }

  Player::update_fire_timer();
  ChaserBoss::has_counted_ = false;
  auto sort_ships = [](const Ship* a, const Ship* b) {
    return a->shape().centre.x - a->bounding_width() < b->shape().centre.x - b->bounding_width();
  };
  std::stable_sort(collisions_.begin(), collisions_.end(), sort_ships);
  for (std::size_t i = 0; i < ships_.size(); ++i) {
    if (!ships_[i]->is_destroyed()) {
      ships_[i]->update();
    }
  }
  for (auto& particle : particles_) {
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
    vec2 v = ships_[0]->shape().centre;
    ships_[0]->shape().centre = it->second.first;
    ships_[0]->explosion(0xffffffff);
    ships_[0]->explosion(it->second.second, 16);
    ships_[0]->explosion(0xffffffff, 24);
    ships_[0]->explosion(it->second.second, 32);
    ships_[0]->shape().centre = v;
    it = Boss::fireworks_.erase(it);
  }

  for (auto it = ships_.begin(); it != ships_.end();) {
    if (!(*it)->is_destroyed()) {
      ++it;
      continue;
    }

    if ((*it)->type() & Ship::kShipEnemy) {
      overmind_->on_enemy_destroy(**it);
    }
    for (auto jt = collisions_.begin(); jt != collisions_.end();) {
      if (it->get() == *jt) {
        jt = collisions_.erase(jt);
        continue;
      }
      ++jt;
    }

    it = ships_.erase(it);
  }

  for (auto it = particles_.begin(); it != particles_.end();) {
    if (it->destroy) {
      it = particles_.erase(it);
      continue;
    }
    ++it;
  }
  overmind_->update();

  if (!kill_timer_ &&
      ((killed_players() == (std::int32_t)players().size() && !get_lives()) ||
       (mode() == game_mode::kBoss && overmind_->get_killed_bosses() >= 6))) {
    kill_timer_ = 100;
  }
  if (kill_timer_) {
    kill_timer_--;
    if (!kill_timer_) {
      game_over_ = true;
    }
  }
}

void SimState::render(Lib& lib) const {
  boss_hp_bar_.reset();
  Stars::render(lib);
  for (const auto& particle : particles_) {
    lib.render_line_rect(particle.position + fvec2{1, 1}, particle.position - fvec2{1, 1},
                         particle.colour);
  }
  for (std::size_t i = player_list_.size(); i < ships_.size(); ++i) {
    ships_[i]->render();
  }
  for (const auto& ship : player_list_) {
    ship->render();
  }

  for (std::size_t i = 0; i < ships_.size() + Boss::warnings_.size(); ++i) {
    if (i < ships_.size() && !(ships_[i]->type() & Ship::kShipEnemy)) {
      continue;
    }
    fvec2 v = to_float(i < ships_.size() ? ships_[i]->shape().centre
                                         : Boss::warnings_[i - ships_.size()]);

    if (v.x < -4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(v.x + Lib::kWidth, 0.f) / Lib::kWidth);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2{0.f, v.y}, fvec2{6, v.y - 3}, a);
      lib.render_line(fvec2{6.f, v.y - 3}, fvec2{6, v.y + 3}, a);
      lib.render_line(fvec2{6.f, v.y + 3}, fvec2{0, v.y}, a);
    }
    if (v.x >= Lib::kWidth + 4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(2 * Lib::kWidth - v.x, 0.f) / Lib::kWidth);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2{float{Lib::kWidth}, v.y}, fvec2{Lib::kWidth - 6.f, v.y - 3}, a);
      lib.render_line(fvec2{Lib::kWidth - 6, v.y - 3}, fvec2{Lib::kWidth - 6.f, v.y + 3}, a);
      lib.render_line(fvec2{Lib::kWidth - 6, v.y + 3}, fvec2{float{Lib::kWidth}, v.y}, a);
    }
    if (v.y < -4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(v.y + Lib::kHeight, 0.f) / Lib::kHeight);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2{v.x, 0.f}, fvec2{v.x - 3, 6.f}, a);
      lib.render_line(fvec2{v.x - 3, 6.f}, fvec2{v.x + 3, 6.f}, a);
      lib.render_line(fvec2{v.x + 3, 6.f}, fvec2{v.x, 0.f}, a);
    }
    if (v.y >= Lib::kHeight + 4) {
      auto a = static_cast<std::int32_t>(
          .5f + float{0x1} + float{0x9} * std::max(2 * Lib::kHeight - v.y, 0.f) / Lib::kHeight);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2{v.x, float{Lib::kHeight}}, fvec2{v.x - 3, Lib::kHeight - 6.f}, a);
      lib.render_line(fvec2{v.x - 3, Lib::kHeight - 6.f}, fvec2{v.x + 3, Lib::kHeight - 6.f}, a);
      lib.render_line(fvec2{v.x + 3, Lib::kHeight - 6.f}, fvec2{v.x, float{Lib::kHeight}}, a);
    }
  }
}

void SimState::write_replay(const std::string& team_name, std::int64_t score) const {
  if (replay_recording_) {
    replay_.write(lib_.filesystem(), team_name, score);
  }
}

Lib& SimState::lib() {
  return lib_;
}

game_mode SimState::mode() const {
  return game_mode(replay_.replay.game_mode());
}

void SimState::add_ship(std::unique_ptr<Ship> ship) {
  ship->set_game(*this);
  if (ship->type() & Ship::kShipEnemy) {
    overmind_->on_enemy_create(*ship);
  }
  if (ship->bounding_width() > 1) {
    collisions_.push_back(ship.get());
  }
  ships_.emplace_back(std::move(ship));
}

void SimState::add_particle(const Particle& particle) {
  particles_.emplace_back(particle);
}

std::int32_t SimState::get_non_wall_count() const {
  return overmind_->count_non_wall_enemies();
}

SimState::ship_list SimState::all_ships(std::int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : ships_) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

SimState::ship_list
SimState::ships_in_radius(const vec2& point, fixed radius, std::int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : ships_) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->shape().centre - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool SimState::any_collision(const vec2& point, std::int32_t category) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : collisions_) {
    fixed sx = collision->shape().centre.x;
    fixed sy = collision->shape().centre.y;
    fixed w = collision->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (collision->check_point(point, category)) {
      return true;
    }
  }
  return false;
}

SimState::ship_list SimState::collision_list(const vec2& point, std::int32_t category) const {
  ship_list r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : collisions_) {
    fixed sx = collision->shape().centre.x;
    fixed sy = collision->shape().centre.y;
    fixed w = collision->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (collision->check_point(point, category)) {
      r.push_back(collision);
    }
  }
  return r;
}

std::int32_t SimState::alive_players() const {
  return players().size() - killed_players();
}

std::int32_t SimState::killed_players() const {
  return Player::killed_players();
}

const SimState::ship_list& SimState::players() const {
  return player_list_;
}

Player* SimState::nearest_player(const vec2& point) const {
  Ship* ship = nullptr;
  Ship* dead = nullptr;
  fixed ship_dist = 0;
  fixed dead_dist = 0;

  for (Ship* s : player_list_) {
    fixed d = (s->shape().centre - point).length_squared();
    if ((d < ship_dist || !ship) && !((Player*)s)->is_killed()) {
      ship_dist = d;
      ship = s;
    }
    if (d < dead_dist || !dead) {
      dead_dist = d;
      dead = s;
    }
  }
  return (Player*)(ship ? ship : dead);
}

bool SimState::game_over() const {
  return game_over_;
}

void SimState::add_life() {
  ++lives_;
}

void SimState::sub_life() {
  if (lives_) {
    lives_--;
  }
}

std::int32_t SimState::get_lives() const {
  return lives_;
}

void SimState::render_hp_bar(float fill) {
  boss_hp_bar_ = fill;
}

void SimState::set_boss_killed(boss_list boss) {
  if (!replay_recording_) {
    return;
  }
  if (boss == BOSS_3A || (mode() != game_mode::kBoss && mode() != game_mode::kNormal)) {
    save_.hard_mode_bosses_killed |= boss;
  } else {
    save_.bosses_killed |= boss;
  }
}

SimState::results SimState::get_results() const {
  results r;
  r.is_replay = !replay_recording_;
  if (r.is_replay) {
    auto input = static_cast<ReplayPlayerInput*>(input_.get());
    r.replay_progress =
        static_cast<float>(input->replay_frame) / input->replay.replay.player_frame_size();
  }
  r.mode = mode();
  r.seed = replay_.replay.seed();
  r.elapsed_time = overmind_->get_elapsed_time();
  r.killed_bosses = overmind_->get_killed_bosses();
  r.lives_remaining = get_lives();
  r.overmind_timer = overmind_->get_timer();
  r.boss_hp_bar = boss_hp_bar_;

  for (auto* ship : players()) {
    auto* p = static_cast<Player*>(ship);
    auto& pr = r.players.emplace_back();
    pr.number = p->player_number();
    pr.score = p->score();
    pr.deaths = p->deaths();
  }
  return r;
}

SimState::SimState(Lib& lib, SaveData& save, std::int32_t* frame_count, Replay&& replay,
                   bool replay_recording)
: lib_{lib}
, save_{save}
, frame_count_{frame_count}
, replay_{replay}
, replay_recording_{replay_recording} {
  static constexpr std::int32_t kStartingLives = 2;
  static constexpr std::int32_t kBossModeLives = 1;
  z::seed((std::int32_t)replay_.replay.seed());
  if (replay_recording_) {
    input_ = std::make_unique<LibPlayerInput>(lib, replay_);
  } else {
    input_ = std::make_unique<ReplayPlayerInput>(replay_);
  }

  lives_ = mode() == game_mode::kBoss ? replay_.replay.players() * kBossModeLives : kStartingLives;
  *frame_count_ = mode() == game_mode::kFast ? 2 : 1;

  Stars::clear();
  for (std::int32_t i = 0; i < replay_.replay.players(); ++i) {
    vec2 v((1 + i) * Lib::kWidth / (1 + replay_.replay.players()), Lib::kHeight / 2);
    auto p = std::make_unique<Player>(*input_, v, i);
    player_list_.push_back(p.get());
    add_ship(std::move(p));
  }
  overmind_ = std::make_unique<Overmind>(*this, replay_.replay.can_face_secret_boss());
}