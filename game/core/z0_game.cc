#include "game/core/z0_game.h"
#include "game/logic/boss/chaser.h"
#include "game/core/lib.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/stars.h"
#include <algorithm>
#include <iostream>

namespace {

void render_panel(Lib& lib, const fvec2& low, const fvec2& hi) {
  fvec2 tlow(low.x * Lib::TEXT_WIDTH, low.y * Lib::TEXT_HEIGHT);
  fvec2 thi(hi.x * Lib::TEXT_WIDTH, hi.y * Lib::TEXT_HEIGHT);
  lib.render_rect(tlow, thi, z0Game::PANEL_BACK);
  lib.render_rect(tlow, thi, z0Game::PANEL_TEXT, 4);
}

std::string convert_to_time(int64_t score) {
  if (score == 0) {
    return "--:--";
  }
  int64_t mins = 0;
  while (score >= 60 * 60 && mins < 99) {
    score -= 60 * 60;
    ++mins;
  }
  int64_t secs = score / 60;

  std::stringstream r;
  if (mins < 10) {
    r << "0";
  }
  r << mins << ":";
  if (secs < 10) {
    r << "0";
  }
  r << secs;
  return r.str();
}

// End anonymous namespace.
}  // namespace

PauseModal::PauseModal(output_t* output, Settings& settings)
: Modal(true, false), _output(output), _settings(settings), _selection(0) {
  *output = CONTINUE;
}

void PauseModal::update(Lib& lib) {
  int32_t t = _selection;
  if (lib.is_key_pressed(Lib::KEY_UP)) {
    _selection = std::max(0, _selection - 1);
  }
  if (lib.is_key_pressed(Lib::KEY_DOWN)) {
    _selection = std::min(2, _selection + 1);
  }
  if (t != _selection) {
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }

  bool accept = lib.is_key_pressed(Lib::KEY_ACCEPT) || lib.is_key_pressed(Lib::KEY_MENU);
  if (accept && _selection == 0) {
    quit();
  } else if (accept && _selection == 1) {
    *_output = END_GAME;
    quit();
  } else if (accept && _selection == 2) {
    _settings.volume = std::min(fixed(100), 1 + _settings.volume);
    _settings.save();
    lib.set_volume(_settings.volume.to_int());
  }
  if (accept) {
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
  }

  if (lib.is_key_pressed(Lib::KEY_CANCEL)) {
    quit();
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
  }

  fixed v = _settings.volume;
  if (_selection == 2 && lib.is_key_pressed(Lib::KEY_LEFT)) {
    _settings.volume = std::max(fixed(0), _settings.volume - 1);
  }
  if (_selection == 2 && lib.is_key_pressed(Lib::KEY_RIGHT)) {
    _settings.volume = std::min(fixed(100), _settings.volume + 1);
  }
  if (v != _settings.volume) {
    lib.set_volume(_settings.volume.to_int());
    _settings.save();
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }
}

void PauseModal::render(Lib& lib) const {
  render_panel(lib, fvec2(3.f, 3.f), fvec2(15.f, 14.f));

  lib.render_text(fvec2(4.f, 4.f), "PAUSED", z0Game::PANEL_TEXT);
  lib.render_text(fvec2(6.f, 8.f), "CONTINUE", z0Game::PANEL_TEXT);
  lib.render_text(fvec2(6.f, 10.f), "END GAME", z0Game::PANEL_TEXT);
  lib.render_text(fvec2(6.f, 12.f), "VOL.", z0Game::PANEL_TEXT);
  std::stringstream vol;
  int32_t v = _settings.volume.to_int();
  vol << " " << (v < 10 ? " " : "") << v;
  lib.render_text(fvec2(10.f, 12.f), vol.str(), z0Game::PANEL_TEXT);
  if (_selection == 2 && v > 0) {
    lib.render_text(fvec2(5.f, 12.f), "<", z0Game::PANEL_TRAN);
  }
  if (_selection == 2 && v < 100) {
    lib.render_text(fvec2(13.f, 12.f), ">", z0Game::PANEL_TRAN);
  }

  fvec2 low(float(4 * Lib::TEXT_WIDTH + 4), float((8 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
  fvec2 hi(float(5 * Lib::TEXT_WIDTH - 4), float((9 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
  lib.render_rect(low, hi, z0Game::PANEL_TEXT, 1);
}

// Compliments have a max length of 24.
static const std::vector<std::string> COMPLIMENTS{
    " is a swell guy!",         " went absolutely mental!",
    " is the bee's knees.",     " goes down in history!",
    " is old school!",          ", oh, how cool you are!",
    " got the respect!",        " had a cow!",
    " is a major badass.",      " is kickin' rad.",
    " wins a coconut.",         " wins a kipper.",
    " is probably cheating!",   " is totally the best!",
    " ate your face!",          " is feeling kinda funky.",
    " is the cat's pyjamas.",   ", air guitar solo time!",
    ", give us a smile.",       " is a cheeky fellow!",
    " is a slippery customer!", "... that's is a puzzle!"};

static const std::string ALLOWED_CHARS = "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! ";

HighScoreModal::HighScoreModal(SaveData& save, GameModal& game, Overmind& overmind, bool replay,
                               int32_t seed)
: Modal(true, false)
, _save(save)
, _game(game)
, _overmind(overmind)
, _replay(replay)
, _seed(seed)
, _enter_char(0)
, _enter_r(0)
, _enter_time(0)
, _compliment(z::rand_int(COMPLIMENTS.size()))
, _timer(0) {}

void HighScoreModal::update(Lib& lib) {
  ++_timer;
  Mode::mode mode = _game.mode();
  int32_t players = _game.players().size();

#ifdef PLATFORM_SCORE
  std::cout << _seed << "\n"
            << players << "\n"
            << (mode == Mode::BOSS) << "\n"
            << (mode == Mode::HARD) << "\n"
            << (mode == Mode::FAST) << "\n"
            << (mode == Mode::WHAT) << "\n"
            << get_score() << "\n"
            << std::flush;
  throw score_finished{};
#endif

  if (!is_high_score()) {
    if (lib.is_key_pressed(Lib::KEY_MENU)) {
      _game.write_replay("untitled", get_score());
      _save.save();
      lib.play_sound(Lib::SOUND_MENU_ACCEPT);
      quit();
    }
    return;
  }

  _enter_time++;
  if (lib.is_key_pressed(Lib::KEY_ACCEPT) && _enter_name.length() < HighScores::MAX_NAME_LENGTH) {
    _enter_name += ALLOWED_CHARS.substr(_enter_char, 1);
    _enter_time = 0;
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }
  if (lib.is_key_pressed(Lib::KEY_CANCEL) && _enter_name.length() > 0) {
    _enter_name = _enter_name.substr(0, _enter_name.length() - 1);
    _enter_time = 0;
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }
  if (lib.is_key_pressed(Lib::KEY_RIGHT) || lib.is_key_pressed(Lib::KEY_LEFT)) {
    _enter_r = 0;
  }
  if (lib.is_key_held(Lib::KEY_RIGHT) || lib.is_key_held(Lib::KEY_LEFT)) {
    ++_enter_r;
    _enter_time = 16;
  }
  if (lib.is_key_pressed(Lib::KEY_RIGHT) ||
      (lib.is_key_held(Lib::KEY_RIGHT) && _enter_r % 5 == 0 && _enter_r > 5)) {
    _enter_char = (_enter_char + 1) % ALLOWED_CHARS.length();
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }
  if (lib.is_key_pressed(Lib::KEY_LEFT) ||
      (lib.is_key_held(Lib::KEY_LEFT) && _enter_r % 5 == 0 && _enter_r > 5)) {
    _enter_char = (_enter_char + ALLOWED_CHARS.length() - 1) % ALLOWED_CHARS.length();
    lib.play_sound(Lib::SOUND_MENU_CLICK);
  }

  if (lib.is_key_pressed(Lib::KEY_MENU)) {
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
    _save.high_scores.add_score(mode, players - 1, _enter_name, get_score());
    _game.write_replay(_enter_name, get_score());
    _save.save();
    quit();
  }
}

void HighScoreModal::render(Lib& lib) const {
  int32_t players = _game.players().size();
  if (is_high_score()) {
    render_panel(lib, fvec2(3.f, 20.f), fvec2(28.f, 27.f));
    lib.render_text(fvec2(4.f, 21.f), "It's a high score!", z0Game::PANEL_TEXT);
    lib.render_text(fvec2(4.f, 23.f),
                    players == 1 ? "Enter name:" : "Enter team name:", z0Game::PANEL_TEXT);
    lib.render_text(fvec2(6.f, 25.f), _enter_name, z0Game::PANEL_TEXT);
    if ((_enter_time / 16) % 2 && _enter_name.length() < HighScores::MAX_NAME_LENGTH) {
      lib.render_text(fvec2(6.f + _enter_name.length(), 25.f), ALLOWED_CHARS.substr(_enter_char, 1),
                      0xbbbbbbff);
    }
    fvec2 low(float(4 * Lib::TEXT_WIDTH + 4), float(25 * Lib::TEXT_HEIGHT + 4));
    fvec2 hi(float(5 * Lib::TEXT_WIDTH - 4), float(26 * Lib::TEXT_HEIGHT - 4));
    lib.render_rect(low, hi, z0Game::PANEL_TEXT, 1);
  }

  if (_game.mode() == Mode::BOSS) {
    int32_t extra_lives = _game.get_lives();
    bool b = extra_lives > 0 && _overmind.get_killed_bosses() >= 6;

    long score = _overmind.get_elapsed_time();
    if (b) {
      score -= 10 * extra_lives;
    }
    if (score <= 0) {
      score = 1;
    }

    render_panel(lib, fvec2(3.f, 3.f), fvec2(37.f, b ? 10.f : 8.f));
    if (b) {
      std::stringstream ss;
      ss << (extra_lives * 10) << "-second extra-life bonus!";
      lib.render_text(fvec2(4.f, 4.f), ss.str(), z0Game::PANEL_TEXT);
    }

    lib.render_text(fvec2(4.f, b ? 6.f : 4.f), "TIME ELAPSED: " + convert_to_time(score),
                    z0Game::PANEL_TEXT);
    std::stringstream ss;
    ss << "BOSS DESTROY: " << _overmind.get_killed_bosses();
    lib.render_text(fvec2(4.f, b ? 8.f : 6.f), ss.str(), z0Game::PANEL_TEXT);
    return;
  }

  render_panel(lib, fvec2(3.f, 3.f), fvec2(37.f, 8.f + 2 * players + (players > 1 ? 2 : 0)));

  std::stringstream ss;
  ss << get_score();
  std::string score = ss.str();
  if (score.length() > HighScores::MAX_SCORE_LENGTH) {
    score = score.substr(0, HighScores::MAX_SCORE_LENGTH);
  }
  lib.render_text(fvec2(4.f, 4.f), "TOTAL SCORE: " + score, z0Game::PANEL_TEXT);

  for (int32_t i = 0; i < players; ++i) {
    std::stringstream sss;
    if (_timer % 600 < 300) {
      sss << ((Player*)_game.players()[i])->score();
    } else {
      int32_t deaths = ((Player*)_game.players()[i])->deaths();
      sss << deaths << " death" << (deaths != 1 ? "s" : "");
    }
    score = sss.str();
    if (score.length() > HighScores::MAX_SCORE_LENGTH) {
      score = score.substr(0, HighScores::MAX_SCORE_LENGTH);
    }

    std::stringstream ssp;
    ssp << "PLAYER " << (i + 1) << ":";
    lib.render_text(fvec2(4.f, 8.f + 2 * i), ssp.str(), z0Game::PANEL_TEXT);
    lib.render_text(fvec2(14.f, 8.f + 2 * i), score, Player::player_colour(i));
  }

  if (players <= 1) {
    return;
  }

  bool first = true;
  int64_t max = 0;
  std::size_t best = 0;
  for (Ship* ship : _game.players()) {
    Player* p = (Player*)ship;
    if (first || p->score() > max) {
      max = p->score();
      best = p->player_number();
    }
    first = false;
  }

  if (get_score() > 0) {
    std::stringstream s;
    s << "PLAYER " << (best + 1);
    lib.render_text(fvec2(4.f, 8.f + 2 * players), s.str(), Player::player_colour(best));

    std::string compliment = COMPLIMENTS[_compliment];
    lib.render_text(fvec2(12.f, 8.f + 2 * players), compliment, z0Game::PANEL_TEXT);
  } else {
    lib.render_text(fvec2(4.f, 8.f + 2 * players), "Oh dear!", z0Game::PANEL_TEXT);
  }
}

int64_t HighScoreModal::get_score() const {
  if (_game.mode() == Mode::BOSS) {
    bool won = _overmind.get_killed_bosses() >= 6 && _overmind.get_elapsed_time() != 0;
    return !won ? 0l : std::max(1l, _overmind.get_elapsed_time() - 600l * _game.get_lives());
  }
  int64_t total = 0;
  for (Ship* ship : _game.players()) {
    total += ((Player*)ship)->score();
  }
  return total;
}

bool HighScoreModal::is_high_score() const {
  return !_replay &&
      _save.high_scores.is_high_score(_game.mode(), _game.players().size() - 1, get_score());
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count,
                     Mode::mode mode, int32_t player_count, bool can_face_secret_boss)
: GameModal(lib, save, settings, frame_count, Replay(mode, player_count, can_face_secret_boss),
            true) {}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count,
                     const std::string& replay_path)
: GameModal(lib, save, settings, frame_count, Replay(replay_path), false) {
  lib.set_player_count(_replay.replay.players());
  lib.new_game();
}

GameModal::~GameModal() {
  Stars::clear();
  Boss::_fireworks.clear();
  Boss::_warnings.clear();
  *_frame_count = 1;
}

void GameModal::update(Lib& lib) {
  if (_pause_output == PauseModal::END_GAME) {
    add(new HighScoreModal(_save, *this, *_overmind, !_replay_recording, _replay.replay.seed()));
    *_frame_count = 1;
    quit();
    return;
  }
  Boss::_warnings.clear();
  lib.capture_mouse(true);

  lib.set_colour_cycle(mode() == Mode::HARD       ? 128
                           : mode() == Mode::FAST ? 192
                           : mode() == Mode::WHAT ? (lib.get_colour_cycle() + 1) % 256
                                                  : 0);
  if (_replay_recording) {
    *_frame_count = mode() == Mode::FAST ? 2 : 1;
  }

  int32_t controllers = 0;
  for (std::size_t i = 0; i < players().size(); i++) {
    controllers += lib.get_pad_type(i);
  }
  if (controllers < _controllers_connected && !_controllers_dialog && _replay_recording) {
    _controllers_dialog = true;
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
  }
  _controllers_connected = controllers;

  if (_controllers_dialog) {
    if (lib.is_key_pressed(Lib::KEY_MENU) || lib.is_key_pressed(Lib::KEY_ACCEPT)) {
      _controllers_dialog = false;
      lib.play_sound(Lib::SOUND_MENU_ACCEPT);
      for (std::size_t i = 0; i < players().size(); i++) {
        lib.rumble(i, 10);
      }
    }
    return;
  }

  if (lib.is_key_pressed(Lib::KEY_MENU)) {
    add(new PauseModal(&_pause_output, _settings));
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
    return;
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
      add(new HighScoreModal(_save, *this, *_overmind, !_replay_recording, _replay.replay.seed()));
      *_frame_count = 1;
      lib.play_sound(Lib::SOUND_MENU_ACCEPT);
      quit();
    }
  }
}

void GameModal::render(Lib& lib) const {
  _show_hp_bar = false;
  _fill_hp_bar = 0;
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

  if (_controllers_dialog) {
    render_panel(lib, fvec2(3.f, 3.f), fvec2(32.f, 8.f + 2 * players().size()));
    lib.render_text(fvec2(4.f, 4.f), "CONTROLLERS FOUND", z0Game::PANEL_TEXT);

    for (std::size_t i = 0; i < players().size(); i++) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      lib.render_text(fvec2(4.f, 8.f + 2 * i), ss.str(), z0Game::PANEL_TEXT);

      std::stringstream ss2;
      int32_t pads = lib.get_pad_type(i);
      if (!_replay_recording) {
        ss2 << "REPLAY";
        pads = 1;
      } else {
        if (!pads) {
          ss2 << "NONE";
        }
        if (pads & Lib::PAD_GAMEPAD) {
          ss2 << "GAMEPAD";
          if (pads & Lib::PAD_KEYMOUSE) {
            ss2 << ", KB/MOUSE";
          }
        } else if (pads & Lib::PAD_KEYMOUSE) {
          ss2 << "MOUSE & KEYBOARD";
        }
      }

      lib.render_text(fvec2(14.f, 8.f + 2 * i), ss2.str(),
                      pads ? Player::player_colour(i) : z0Game::PANEL_TEXT);
    }
    return;
  }

  std::stringstream ss;
  ss << _lives << " live(s)";
  if (mode() != Mode::BOSS && _overmind->get_timer() >= 0) {
    int32_t t = int32_t(0.5f + _overmind->get_timer() / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  lib.render_text(fvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                        Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f),
                  ss.str(), z0Game::PANEL_TRAN);

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
  if (mode() == Mode::BOSS) {
    std::stringstream sst;
    sst << convert_to_time(_overmind->get_elapsed_time());
    lib.render_text(fvec2(Lib::WIDTH / (2 * Lib::TEXT_WIDTH) - sst.str().length() - 1.f, 1.f),
                    sst.str(), z0Game::PANEL_TRAN);
  }

  if (_show_hp_bar) {
    int32_t x = mode() == Mode::BOSS ? 48 : 0;
    lib.render_rect(fvec2(x + Lib::WIDTH / 2 - 48.f, 16.f), fvec2(x + Lib::WIDTH / 2 + 48.f, 32.f),
                    z0Game::PANEL_TRAN, 2);

    lib.render_rect(fvec2(x + Lib::WIDTH / 2 - 44.f, 16.f + 4),
                    fvec2(x + Lib::WIDTH / 2 - 44.f + 88.f * _fill_hp_bar, 32.f - 4),
                    z0Game::PANEL_TRAN);
  }

  if (!_replay_recording) {
    auto input = (ReplayPlayerInput*)_input.get();
    int32_t x = mode() == Mode::FAST ? *_frame_count / 2 : *_frame_count;
    std::stringstream ss;
    ss << x << "X "
       << int32_t(100 * float(input->replay_frame) / input->replay.replay.player_frame_size())
       << "%";

    lib.render_text(fvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                          Lib::HEIGHT / Lib::TEXT_HEIGHT - 3.f),
                    ss.str(), z0Game::PANEL_TRAN);
  }
}

void GameModal::write_replay(const std::string& team_name, int64_t score) const {
  if (_replay_recording) {
    _replay.write(team_name, score);
  }
}

Lib& GameModal::lib() {
  return _lib;
}

Mode::mode GameModal::mode() const {
  return Mode::mode(_replay.replay.game_mode());
}

void GameModal::add_ship(Ship* ship) {
  ship->set_game(*this);
  if (ship->type() & Ship::SHIP_ENEMY) {
    _overmind->on_enemy_create(*ship);
  }
  _ships.emplace_back(ship);

  if (ship->bounding_width() > 1) {
    _collisions.push_back(ship);
  }
}

void GameModal::add_particle(const Particle& particle) {
  _particles.emplace_back(particle);
}

int32_t GameModal::get_non_wall_count() const {
  return _overmind->count_non_wall_enemies();
}

GameModal::ship_list GameModal::all_ships(int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : _ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

GameModal::ship_list
GameModal::ships_in_radius(const vec2& point, fixed radius, int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : _ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->shape().centre - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool GameModal::any_collision(const vec2& point, int32_t category) const {
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

GameModal::ship_list GameModal::collision_list(const vec2& point, int32_t category) const {
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

int32_t GameModal::alive_players() const {
  return players().size() - killed_players();
}

int32_t GameModal::killed_players() const {
  return Player::killed_players();
}

const GameModal::ship_list& GameModal::players() const {
  return _player_list;
}

Player* GameModal::nearest_player(const vec2& point) const {
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

void GameModal::add_life() {
  _lives++;
}

void GameModal::sub_life() {
  if (_lives) {
    _lives--;
  }
}

int32_t GameModal::get_lives() const {
  return _lives;
}

void GameModal::render_hp_bar(float fill) {
  _show_hp_bar = true;
  _fill_hp_bar = fill;
}

void GameModal::set_boss_killed(boss_list boss) {
  if (!_replay_recording) {
    return;
  }
  if (boss == BOSS_3A || (mode() != Mode::BOSS && mode() != Mode::NORMAL)) {
    _save.hard_mode_bosses_killed |= boss;
  } else {
    _save.bosses_killed |= boss;
  }
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, int32_t* frame_count,
                     Replay&& replay, bool replay_recording)
: Modal(true, true)
, _lib(lib)
, _save(save)
, _settings(settings)
, _pause_output(PauseModal::CONTINUE)
, _frame_count(frame_count)
, _kill_timer(0)
, _replay(replay)
, _replay_recording(replay_recording)
, _input(_replay_recording ? (PlayerInput*)new LibPlayerInput(lib, _replay)
                           : (PlayerInput*)new ReplayPlayerInput(_replay))
, _controllers_connected(0)
, _controllers_dialog(true)
, _show_hp_bar(false)
, _fill_hp_bar(0) {
  static const int32_t STARTING_LIVES = 2;
  static const int32_t BOSSMODE_LIVES = 1;
  z::seed((int32_t)_replay.replay.seed());
  _lives = mode() == Mode::BOSS ? _replay.replay.players() * BOSSMODE_LIVES : STARTING_LIVES;
  *_frame_count = mode() == Mode::FAST ? 2 : 1;

  Stars::clear();
  for (int32_t i = 0; i < _replay.replay.players(); ++i) {
    vec2 v((1 + i) * Lib::WIDTH / (1 + _replay.replay.players()), Lib::HEIGHT / 2);
    Player* p = new Player(*_input, v, i);
    add_ship(p);
    _player_list.push_back(p);
  }
  _overmind.reset(new Overmind(*this, _replay.replay.can_face_secret_boss()));
}

z0Game::z0Game(Lib& lib, const std::vector<std::string>& args)
: _lib(lib)
, _frame_count(1)
, _menu_select(MENU_START)
, _player_select(1)
, _mode_select(Mode::BOSS)
, _exit_timer(0) {
  lib.set_volume(_settings.volume.to_int());

  if (!args.empty()) {
    try {
      _modals.add(new GameModal(_lib, _save, _settings, &_frame_count, args[0]));
    } catch (const std::runtime_error& e) {
      _exit_timer = 256;
      _exit_error = e.what();
    }
  }
}

void z0Game::run() {
  while (true) {
    std::size_t f = _frame_count;
    if (!f) {
      continue;
    }

    for (std::size_t i = 0; i < f; ++i) {
#ifdef PLATFORM_SCORE
      _frame_count = 16384;
#endif
      if (_lib.begin_frame() || update()) {
        _lib.end_frame();
        return;
      }
      _lib.end_frame();
    }

    _lib.clear_screen();
    render();
    _lib.render();
  }
}

bool z0Game::update() {
  if (_exit_timer) {
    _exit_timer--;
    return !_exit_timer;
  }

  for (int32_t i = 0; i < PLAYERS; i++) {
    if (lib().is_key_held(i, Lib::KEY_FIRE) && lib().is_key_held(i, Lib::KEY_BOMB) &&
        lib().is_key_pressed(Lib::KEY_MENU)) {
      lib().take_screenshot();
      break;
    }
  }
  lib().capture_mouse(false);

  if (_modals.update(_lib)) {
    return false;
  }

  menu_t t = _menu_select;
  if (lib().is_key_pressed(Lib::KEY_UP)) {
    _menu_select = std::max(menu_t(_menu_select - 1),
                            mode_unlocked() >= Mode::BOSS ? MENU_SPECIAL : MENU_START);
  }
  if (lib().is_key_pressed(Lib::KEY_DOWN)) {
    _menu_select = std::min(MENU_QUIT, menu_t(_menu_select + 1));
  }
  if (t != _menu_select) {
    lib().play_sound(Lib::SOUND_MENU_CLICK);
  }

  if (_menu_select == MENU_PLAYERS) {
    int32_t t = _player_select;
    if (lib().is_key_pressed(Lib::KEY_LEFT)) {
      _player_select = std::max(1, _player_select - 1);
    }
    if (lib().is_key_pressed(Lib::KEY_RIGHT)) {
      _player_select = std::min(PLAYERS, _player_select + 1);
    }
    if (lib().is_key_pressed(Lib::KEY_ACCEPT) || lib().is_key_pressed(Lib::KEY_MENU)) {
      _player_select = 1 + _player_select % PLAYERS;
    }
    if (t != _player_select) {
      lib().play_sound(Lib::SOUND_MENU_CLICK);
    }
    lib().set_player_count(_player_select);
  }

  if (_menu_select == MENU_SPECIAL) {
    Mode::mode t = _mode_select;
    if (lib().is_key_pressed(Lib::KEY_LEFT)) {
      _mode_select = std::max(Mode::BOSS, Mode::mode(_mode_select - 1));
    }
    if (lib().is_key_pressed(Lib::KEY_RIGHT)) {
      _mode_select = std::min(mode_unlocked(), Mode::mode(_mode_select + 1));
    }
    if (t != _mode_select) {
      lib().play_sound(Lib::SOUND_MENU_CLICK);
    }
  }

  if (lib().is_key_pressed(Lib::KEY_ACCEPT) || lib().is_key_pressed(Lib::KEY_MENU)) {
    if (_menu_select == MENU_START) {
      lib().new_game();
      _modals.add(new GameModal(_lib, _save, _settings, &_frame_count, Mode::NORMAL, _player_select,
                                mode_unlocked() >= Mode::FAST));
    } else if (_menu_select == MENU_QUIT) {
      _exit_timer = 2;
    } else if (_menu_select == MENU_SPECIAL) {
      lib().new_game();
      _modals.add(new GameModal(_lib, _save, _settings, &_frame_count, _mode_select, _player_select,
                                mode_unlocked() >= Mode::FAST));
    }
    if (_menu_select != MENU_PLAYERS) {
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }
  }

  if (_menu_select >= MENU_START || _mode_select == Mode::BOSS) {
    lib().set_colour_cycle(0);
  } else if (_mode_select == Mode::HARD) {
    lib().set_colour_cycle(128);
  } else if (_mode_select == Mode::FAST) {
    lib().set_colour_cycle(192);
  } else if (_mode_select == Mode::WHAT) {
    lib().set_colour_cycle((lib().get_colour_cycle() + 1) % 256);
  }
  return false;
}

void z0Game::render() const {
  if (_exit_timer > 0 && !_exit_error.empty()) {
    int32_t y = 2;
    std::string s = _exit_error;
    std::size_t i;
    while ((i = s.find_first_of("\n")) != std::string::npos) {
      std::string t = s.substr(0, i);
      s = s.substr(i + 1);
      lib().render_text(fvec2(2, float(y)), t, PANEL_TEXT);
      ++y;
    }
    lib().render_text(fvec2(2, float(y)), s, PANEL_TEXT);
    return;
  }

  if (_modals.render(_lib)) {
    return;
  }

  render_panel(lib(), fvec2(3.f, 3.f), fvec2(19.f, 14.f));

  lib().render_text(fvec2(37.f - 16, 3.f), "coded by: SEiKEN", PANEL_TEXT);
  lib().render_text(fvec2(37.f - 16, 4.f), "stu@seiken.co.uk", PANEL_TEXT);
  lib().render_text(fvec2(37.f - 9, 6.f), "-testers-", PANEL_TEXT);
  lib().render_text(fvec2(37.f - 9, 7.f), "MATT BELL", PANEL_TEXT);
  lib().render_text(fvec2(37.f - 9, 8.f), "RUFUZZZZZ", PANEL_TEXT);
  lib().render_text(fvec2(37.f - 9, 9.f), "SHADOW1W2", PANEL_TEXT);

  std::string b = "BOSSES:  ";
  int32_t bb = mode_unlocked() >= Mode::HARD ? _save.hard_mode_bosses_killed : _save.bosses_killed;
  b += bb & GameModal::BOSS_1A ? "X" : "-";
  b += bb & GameModal::BOSS_1B ? "X" : "-";
  b += bb & GameModal::BOSS_1C ? "X" : "-";
  b += bb & GameModal::BOSS_3A ? "X" : " ";
  b += bb & GameModal::BOSS_2A ? "X" : "-";
  b += bb & GameModal::BOSS_2B ? "X" : "-";
  b += bb & GameModal::BOSS_2C ? "X" : "-";
  lib().render_text(fvec2(37.f - 16, 13.f), b, PANEL_TEXT);

  lib().render_text(fvec2(4.f, 4.f), "WiiSPACE", PANEL_TEXT);
  lib().render_text(fvec2(6.f, 8.f), "START GAME", PANEL_TEXT);
  lib().render_text(fvec2(6.f, 10.f), "PLAYERS", PANEL_TEXT);
  lib().render_text(fvec2(6.f, 12.f), "EXiT", PANEL_TEXT);

  if (mode_unlocked() >= Mode::BOSS) {
    std::string str = _mode_select == Mode::BOSS ? "BOSS MODE"
        : _mode_select == Mode::HARD             ? "HARD MODE"
        : _mode_select == Mode::FAST             ? "FAST MODE"
                                                 : "W-HAT MODE";
    lib().render_text(fvec2(6.f, 6.f), str, PANEL_TEXT);
  }
  if (_menu_select == MENU_SPECIAL && _mode_select > Mode::BOSS) {
    lib().render_text(fvec2(5.f, 6.f), "<", PANEL_TRAN);
  }
  if (_menu_select == MENU_SPECIAL && _mode_select < mode_unlocked()) {
    lib().render_text(fvec2(6.f, 6.f), "         >", PANEL_TRAN);
  }

  fvec2 low(float(4 * Lib::TEXT_WIDTH + 4), float((6 + 2 * _menu_select) * Lib::TEXT_HEIGHT + 4));
  fvec2 hi(float(5 * Lib::TEXT_WIDTH - 4), float((7 + 2 * _menu_select) * Lib::TEXT_HEIGHT - 4));
  lib().render_rect(low, hi, PANEL_TEXT, 1);

  if (_player_select > 1 && _menu_select == MENU_PLAYERS) {
    lib().render_text(fvec2(5.f, 10.f), "<", PANEL_TRAN);
  }
  if (_player_select < 4 && _menu_select == MENU_PLAYERS) {
    lib().render_text(fvec2(14.f + _player_select, 10.f), ">", PANEL_TRAN);
  }
  for (int32_t i = 0; i < _player_select; ++i) {
    std::stringstream ss;
    ss << (i + 1);
    lib().render_text(fvec2(14.f + i, 10.f), ss.str(), Player::player_colour(i));
  }

  render_panel(lib(), fvec2(3.f, 15.f), fvec2(37.f, 27.f));

  std::stringstream ss;
  ss << _player_select;
  std::string s = "HiGH SCORES    ";
  s += _menu_select == MENU_SPECIAL
      ? (_mode_select == Mode::BOSS       ? "BOSS MODE"
             : _mode_select == Mode::HARD ? "HARD MODE (" + ss.str() + "P)"
             : _mode_select == Mode::FAST ? "FAST MODE (" + ss.str() + "P)"
                                          : "W-HAT MODE (" + ss.str() + "P)")
      : _player_select == 1 ? "ONE PLAYER"
      : _player_select == 2 ? "TWO PLAYERS"
      : _player_select == 3 ? "THREE PLAYERS"
      : _player_select == 4 ? "FOUR PLAYERS"
                            : "";
  lib().render_text(fvec2(4.f, 16.f), s, PANEL_TEXT);

  if (_menu_select == MENU_SPECIAL && _mode_select == Mode::BOSS) {
    lib().render_text(fvec2(4.f, 18.f), "ONE PLAYER", PANEL_TEXT);
    lib().render_text(fvec2(4.f, 20.f), "TWO PLAYERS", PANEL_TEXT);
    lib().render_text(fvec2(4.f, 22.f), "THREE PLAYERS", PANEL_TEXT);
    lib().render_text(fvec2(4.f, 24.f), "FOUR PLAYERS", PANEL_TEXT);

    for (std::size_t i = 0; i < PLAYERS; i++) {
      auto& s = _save.high_scores.get(Mode::BOSS, i, 0);
      std::string score = convert_to_time(s.score).substr(0, HighScores::MAX_NAME_LENGTH);
      std::string name = s.name.substr(HighScores::MAX_NAME_LENGTH);

      lib().render_text(fvec2(19.f, 18.f + i * 2), score, PANEL_TEXT);
      lib().render_text(fvec2(19.f, 19.f + i * 2), name, PANEL_TEXT);
    }
  } else {
    for (std::size_t i = 0; i < HighScores::NUM_SCORES; i++) {
      std::stringstream ssi;
      ssi << (i + 1) << ".";
      lib().render_text(fvec2(4.f, 18.f + i), ssi.str(), PANEL_TEXT);

      auto& s = _save.high_scores.get(_menu_select == MENU_SPECIAL ? _mode_select : Mode::NORMAL,
                                      _player_select - 1, i);
      if (s.score <= 0) {
        continue;
      }

      std::stringstream ss;
      ss << s.score;
      std::string score = ss.str().substr(0, HighScores::MAX_SCORE_LENGTH);
      std::string name = s.name.substr(0, HighScores::MAX_SCORE_LENGTH);

      lib().render_text(fvec2(7.f, 18.f + i), score, PANEL_TEXT);
      lib().render_text(fvec2(19.f, 18.f + i), name, PANEL_TEXT);
    }
  }
}

Mode::mode z0Game::mode_unlocked() const {
  if (!(_save.bosses_killed & 63)) {
    return Mode::NORMAL;
  }
  if (_save.high_scores.boss[0].score == 0 && _save.high_scores.boss[1].score == 0 &&
      _save.high_scores.boss[2].score == 0 && _save.high_scores.boss[3].score == 0) {
    return Mode::BOSS;
  }
  if ((_save.hard_mode_bosses_killed & 63) != 63) {
    return Mode::HARD;
  }
  if ((_save.hard_mode_bosses_killed & 64) != 64) {
    return Mode::FAST;
  }
  return Mode::WHAT;
}
