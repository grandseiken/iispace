#include "game/logic/sim_state.h"
#include "game/core/save.h"
#include "game/logic/boss.h"
#include "game/logic/boss/chaser.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/player_input.h"
#include "game/logic/ship.h"
#include "game/logic/stars.h"

SimState::SimState(Lib& lib, SaveData& save, int32_t* frame_count, Mode::mode mode,
                   int32_t player_count, bool can_face_secret_boss)
: SimState(lib, save, frame_count, Replay(mode, player_count, can_face_secret_boss), true) {}

SimState::SimState(Lib& lib, SaveData& save, int32_t* frame_count, const std::string& replay_path)
: SimState(lib, save, frame_count, Replay(replay_path), false) {
  lib.set_player_count(_replay.replay.players());
  lib.new_game();
}

SimState::~SimState() {
  Stars::clear();
  Boss::_fireworks.clear();
  Boss::_warnings.clear();
  *_frame_count = 1;
}

void SimState::update(Lib& lib) {
  Boss::_warnings.clear();
  lib.set_colour_cycle(mode() == Mode::HARD       ? 128
                           : mode() == Mode::FAST ? 192
                           : mode() == Mode::WHAT ? (lib.get_colour_cycle() + 1) % 256
                                                  : 0);
  if (_replay_recording) {
    *_frame_count = mode() == Mode::FAST ? 2 : 1;
  }

  if (!_replay_recording) {
    if (lib.is_key_pressed(Lib::KEY_BOMB)) {
      *_frame_count *= 2;
    }
    if (lib.is_key_pressed(Lib::KEY_FIRE) && *_frame_count > (mode() == Mode::FAST ? 2 : 1)) {
      *_frame_count /= 2;
    }
  }

  Player::update_fire_timer();
  ChaserBoss::_has_counted = false;
  auto sort_ships = [](const Ship* a, const Ship* b) {
    return a->shape().centre.x - a->bounding_width() < b->shape().centre.x - b->bounding_width();
  };
  std::stable_sort(_collisions.begin(), _collisions.end(), sort_ships);
  for (std::size_t i = 0; i < _ships.size(); ++i) {
    if (!_ships[i]->is_destroyed()) {
      _ships[i]->update();
    }
  }
  for (auto& particle : _particles) {
    if (!particle.destroy) {
      particle.position += particle.velocity;
      if (--particle.timer <= 0) {
        particle.destroy = true;
      }
    }
  }
  for (auto it = Boss::_fireworks.begin(); it != Boss::_fireworks.end();) {
    if (it->first > 0) {
      --(it++)->first;
      continue;
    }
    vec2 v = _ships[0]->shape().centre;
    _ships[0]->shape().centre = it->second.first;
    _ships[0]->explosion(0xffffffff);
    _ships[0]->explosion(it->second.second, 16);
    _ships[0]->explosion(0xffffffff, 24);
    _ships[0]->explosion(it->second.second, 32);
    _ships[0]->shape().centre = v;
    it = Boss::_fireworks.erase(it);
  }

  for (auto it = _ships.begin(); it != _ships.end();) {
    if (!(*it)->is_destroyed()) {
      ++it;
      continue;
    }

    if ((*it)->type() & Ship::SHIP_ENEMY) {
      _overmind->on_enemy_destroy(**it);
    }
    for (auto jt = _collisions.begin(); jt != _collisions.end();) {
      if (it->get() == *jt) {
        jt = _collisions.erase(jt);
        continue;
      }
      ++jt;
    }

    it = _ships.erase(it);
  }

  for (auto it = _particles.begin(); it != _particles.end();) {
    if (it->destroy) {
      it = _particles.erase(it);
      continue;
    }
    ++it;
  }
  _overmind->update();

  if (!_kill_timer &&
      ((killed_players() == (int32_t)players().size() && !get_lives()) ||
       (mode() == Mode::BOSS && _overmind->get_killed_bosses() >= 6))) {
    _kill_timer = 100;
  }
  if (_kill_timer) {
    _kill_timer--;
    if (!_kill_timer) {
      _game_over = true;
    }
  }
}

void SimState::render(Lib& lib) const {
  _boss_hp_bar.reset();
  Stars::render(lib);
  for (const auto& particle : _particles) {
    lib.render_rect(particle.position + fvec2(1, 1), particle.position - fvec2(1, 1),
                    particle.colour);
  }
  for (std::size_t i = _player_list.size(); i < _ships.size(); ++i) {
    _ships[i]->render();
  }
  for (const auto& ship : _player_list) {
    ship->render();
  }

  for (std::size_t i = 0; i < _ships.size() + Boss::_warnings.size(); ++i) {
    if (i < _ships.size() && !(_ships[i]->type() & Ship::SHIP_ENEMY)) {
      continue;
    }
    fvec2 v = to_float(i < _ships.size() ? _ships[i]->shape().centre
                                         : Boss::_warnings[i - _ships.size()]);

    if (v.x < -4) {
      int32_t a =
          int32_t(.5f + float(0x1) + float(0x9) * std::max(v.x + Lib::WIDTH, 0.f) / Lib::WIDTH);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2(0.f, v.y), fvec2(6, v.y - 3), a);
      lib.render_line(fvec2(6.f, v.y - 3), fvec2(6, v.y + 3), a);
      lib.render_line(fvec2(6.f, v.y + 3), fvec2(0, v.y), a);
    }
    if (v.x >= Lib::WIDTH + 4) {
      int32_t a =
          int32_t(.5f + float(0x1) + float(0x9) * std::max(2 * Lib::WIDTH - v.x, 0.f) / Lib::WIDTH);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2(float(Lib::WIDTH), v.y), fvec2(Lib::WIDTH - 6.f, v.y - 3), a);
      lib.render_line(fvec2(Lib::WIDTH - 6, v.y - 3), fvec2(Lib::WIDTH - 6.f, v.y + 3), a);
      lib.render_line(fvec2(Lib::WIDTH - 6, v.y + 3), fvec2(float(Lib::WIDTH), v.y), a);
    }
    if (v.y < -4) {
      int32_t a =
          int32_t(.5f + float(0x1) + float(0x9) * std::max(v.y + Lib::HEIGHT, 0.f) / Lib::HEIGHT);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2(v.x, 0.f), fvec2(v.x - 3, 6.f), a);
      lib.render_line(fvec2(v.x - 3, 6.f), fvec2(v.x + 3, 6.f), a);
      lib.render_line(fvec2(v.x + 3, 6.f), fvec2(v.x, 0.f), a);
    }
    if (v.y >= Lib::HEIGHT + 4) {
      int32_t a = int32_t(.5f + float(0x1) +
                          float(0x9) * std::max(2 * Lib::HEIGHT - v.y, 0.f) / Lib::HEIGHT);
      a |= a << 4;
      a = (a << 8) | (a << 16) | (a << 24) | 0x66;
      lib.render_line(fvec2(v.x, float(Lib::HEIGHT)), fvec2(v.x - 3, Lib::HEIGHT - 6.f), a);
      lib.render_line(fvec2(v.x - 3, Lib::HEIGHT - 6.f), fvec2(v.x + 3, Lib::HEIGHT - 6.f), a);
      lib.render_line(fvec2(v.x + 3, Lib::HEIGHT - 6.f), fvec2(v.x, float(Lib::HEIGHT)), a);
    }
  }
}

void SimState::write_replay(const std::string& team_name, int64_t score) const {
  if (_replay_recording) {
    _replay.write(team_name, score);
  }
}

Lib& SimState::lib() {
  return _lib;
}

Mode::mode SimState::mode() const {
  return Mode::mode(_replay.replay.game_mode());
}

void SimState::add_ship(Ship* ship) {
  ship->set_game(*this);
  if (ship->type() & Ship::SHIP_ENEMY) {
    _overmind->on_enemy_create(*ship);
  }
  _ships.emplace_back(ship);

  if (ship->bounding_width() > 1) {
    _collisions.push_back(ship);
  }
}

void SimState::add_particle(const Particle& particle) {
  _particles.emplace_back(particle);
}

int32_t SimState::get_non_wall_count() const {
  return _overmind->count_non_wall_enemies();
}

SimState::ship_list SimState::all_ships(int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : _ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

SimState::ship_list
SimState::ships_in_radius(const vec2& point, fixed radius, int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : _ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->shape().centre - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool SimState::any_collision(const vec2& point, int32_t category) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : _collisions) {
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

SimState::ship_list SimState::collision_list(const vec2& point, int32_t category) const {
  ship_list r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : _collisions) {
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

int32_t SimState::alive_players() const {
  return players().size() - killed_players();
}

int32_t SimState::killed_players() const {
  return Player::killed_players();
}

const SimState::ship_list& SimState::players() const {
  return _player_list;
}

Player* SimState::nearest_player(const vec2& point) const {
  Ship* ship = nullptr;
  Ship* dead = nullptr;
  fixed ship_dist = 0;
  fixed dead_dist = 0;

  for (Ship* s : _player_list) {
    fixed d = (s->shape().centre - point).length();
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
  return _game_over;
}

void SimState::add_life() {
  _lives++;
}

void SimState::sub_life() {
  if (_lives) {
    _lives--;
  }
}

int32_t SimState::get_lives() const {
  return _lives;
}

void SimState::render_hp_bar(float fill) {
  _boss_hp_bar = fill;
}

void SimState::set_boss_killed(boss_list boss) {
  if (!_replay_recording) {
    return;
  }
  if (boss == BOSS_3A || (mode() != Mode::BOSS && mode() != Mode::NORMAL)) {
    _save.hard_mode_bosses_killed |= boss;
  } else {
    _save.bosses_killed |= boss;
  }
}

SimState::results SimState::get_results() const {
  results r;
  r.is_replay = !_replay_recording;
  if (r.is_replay) {
    auto input = static_cast<ReplayPlayerInput*>(_input.get());
    r.replay_progress =
        static_cast<float>(input->replay_frame) / input->replay.replay.player_frame_size();
  }
  r.mode = mode();
  r.seed = _replay.replay.seed();
  r.elapsed_time = _overmind->get_elapsed_time();
  r.killed_bosses = _overmind->get_killed_bosses();
  r.lives_remaining = get_lives();
  r.overmind_timer = _overmind->get_timer();
  r.boss_hp_bar = _boss_hp_bar;

  for (auto* ship : players()) {
    auto* p = static_cast<Player*>(ship);
    auto& pr = r.players.emplace_back();
    pr.number = p->player_number();
    pr.score = p->score();
    pr.deaths = p->deaths();
  }
  return r;
}

SimState::SimState(Lib& lib, SaveData& save, int32_t* frame_count, Replay&& replay,
                   bool replay_recording)
: _lib{lib}
, _save{save}
, _frame_count{frame_count}
, _replay{replay}
, _replay_recording{replay_recording} {
  static const int32_t STARTING_LIVES = 2;
  static const int32_t BOSSMODE_LIVES = 1;
  z::seed((int32_t)_replay.replay.seed());
  if (_replay_recording) {
    _input = std::make_unique<LibPlayerInput>(lib, _replay);
  } else {
    _input = std::make_unique<ReplayPlayerInput>(_replay);
  }

  _lives = mode() == Mode::BOSS ? _replay.replay.players() * BOSSMODE_LIVES : STARTING_LIVES;
  *_frame_count = mode() == Mode::FAST ? 2 : 1;

  Stars::clear();
  for (int32_t i = 0; i < _replay.replay.players(); ++i) {
    vec2 v((1 + i) * Lib::WIDTH / (1 + _replay.replay.players()), Lib::HEIGHT / 2);
    Player* p = new Player(*_input, v, i);
    add_ship(p);
    _player_list.push_back(p);
  }
  _overmind = std::make_unique<Overmind>(*this, _replay.replay.can_face_secret_boss());
}