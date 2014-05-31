#include "z0_game.h"
#include "lib.h"
#include "player.h"
#include "boss.h"
#include "enemy.h"
#include "overmind.h"
#include "stars.h"
#include <algorithm>
#include <iostream>

const int z0Game::STARTING_LIVES = 2;
const int z0Game::BOSSMODE_LIVES = 1;

z0Game::z0Game(Lib& lib, const std::vector<std::string>& args)
  : _lib(lib)
  , _state(STATE_MENU)
  , _players(1)
  , _lives(0)
  , _mode(NORMAL_MODE)
  , _frame_count(1)
  , _show_hp_bar(false)
  , _fill_hp_bar(0)
  , _selection(0)
  , _special_selection(0)
  , _kill_timer(0)
  , _exit_timer(0)
  , _enter_char(0)
  , _enter_time(0)
  , _score_screen_timer(0)
  , _controllers_connected(0)
  , _controllers_dialog(false)
  , _first_controllers_dialog(false)
  , _overmind(new Overmind(*this))
{
  lib.set_volume(_settings.volume.to_int());
  // Compliments have a max length of 24
  _compliments.push_back(" is a swell guy!");
  _compliments.push_back(" went absolutely mental!");
  _compliments.push_back(" is the bee's knees.");
  _compliments.push_back(" goes down in history!");
  _compliments.push_back(" is old school!");
  _compliments.push_back(", oh, how cool you are!");
  _compliments.push_back(" got the respect!");
  _compliments.push_back(" had a cow!");
  _compliments.push_back(" is a major badass.");
  _compliments.push_back(" is kickin' rad.");
  _compliments.push_back(" wins a coconut.");
  _compliments.push_back(" wins a kipper.");
  _compliments.push_back(" is probably cheating!");
  _compliments.push_back(" is totally the best!");
  _compliments.push_back(" ate your face!");
  _compliments.push_back(" is feeling kinda funky.");
  _compliments.push_back(" is the cat's pyjamas.");
  _compliments.push_back(", air guitar solo time!");
  _compliments.push_back(", give us a smile.");
  _compliments.push_back(" is a cheeky fellow!");
  _compliments.push_back(" is a slippery customer!");
  _compliments.push_back("... that's is a puzzle!");

  if (!args.empty()) {
    Player::replay = Replay(args[0]);
    Player::replay_frame = 0;
    if (Player::replay.okay) {
      _players = Player::replay.players;
      lib.set_player_count(_players);
      new_game(Player::replay.can_face_secret_boss,
               true, game_mode(Player::replay.game_mode));
    }
    else {
      _exit_timer = 256;
    }
  }
}

z0Game::~z0Game()
{
  for (const auto& ship : _ships) {
    if (ship->type() & Ship::SHIP_ENEMY) {
      _overmind->on_enemy_destroy(*ship);
    }
  }
}

void z0Game::run()
{
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

bool z0Game::update()
{
  if (_exit_timer) {
    _exit_timer--;
    return !_exit_timer;
  }

  for (int32_t i = 0; i < Lib::PLAYERS; i++) {
    if (lib().is_key_held(i, Lib::KEY_FIRE) &&
        lib().is_key_held(i, Lib::KEY_BOMB) &&
        lib().is_key_pressed(Lib::KEY_MENU)) {
      lib().take_screenshot();
      break;
    }
  }
  lib().capture_mouse(false);

  // Main menu
  //------------------------------
  if (_state == STATE_MENU) {
    if (lib().is_key_pressed(Lib::KEY_UP)) {
      _selection--;
      if (_selection < 0 && !is_boss_mode_unlocked()) {
        _selection = 0;
      }
      else if (_selection < -1 && is_boss_mode_unlocked()) {
        _selection = -1;
      }
      else {
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().is_key_pressed(Lib::KEY_DOWN)) {
      _selection++;
      if (_selection > 2) {
        _selection = 2;
      }
      else {
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
    }

    if (lib().is_key_pressed(Lib::KEY_ACCEPT) ||
        lib().is_key_pressed(Lib::KEY_MENU)) {
      if (_selection == 0) {
        new_game(is_fast_mode_unlocked(), false, NORMAL_MODE);
      }
      else if (_selection == 1) {
        _players++;
        if (_players > Lib::PLAYERS) {
          _players = 1;
        }
        lib().set_player_count(_players);
      }
      else if (_selection == 2) {
        _exit_timer = 2;
      }
      else if (_selection == -1) {
        new_game(is_fast_mode_unlocked(), false,
                 _special_selection == 0 ? BOSS_MODE :
                 _special_selection == 1 ? HARD_MODE :
                 _special_selection == 2 ? FAST_MODE :
                 _special_selection == 3 ? WHAT_MODE : NORMAL_MODE);
      }

      if (_selection == 1) {
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      else {
        lib().play_sound(Lib::SOUND_MENU_ACCEPT);
      }
    }

    if (_selection == 1) {
      int t = _players;
      if (lib().is_key_pressed(Lib::KEY_LEFT) && _players > 1) {
        _players--;
      }
      else if (lib().is_key_pressed(Lib::KEY_RIGHT) && _players < Lib::PLAYERS) {
        _players++;
      }
      if (t != _players){
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      lib().set_player_count(_players);
    }

    if (_selection == -1) {
      int t = _special_selection;
      if (lib().is_key_pressed(Lib::KEY_LEFT) && _special_selection > 0) {
        _special_selection--;
      }
      else if (lib().is_key_pressed(Lib::KEY_RIGHT)) {
        if ((t == 0 && is_hard_mode_unlocked()) ||
            (t == 1 && is_fast_mode_unlocked()) ||
            (t == 2 && is_what_mode_unlocked())) {
          _special_selection++;
        }
      }
      if (t != _special_selection) {
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
    }

    if (_selection >= 0 || _special_selection == 0) {
      lib().set_colour_cycle(0);
    }
    else if (_special_selection == 1) {
      lib().set_colour_cycle(128);
    }
    else if (_special_selection == 2) {
      lib().set_colour_cycle(192);
    }
    else if (_special_selection == 3) {
      lib().set_colour_cycle((lib().get_colour_cycle() + 1) % 256);
    }
  }
  // Paused
  //------------------------------
  else if (_state == STATE_PAUSE) {
    int t = _selection;
    if (lib().is_key_pressed(Lib::KEY_UP) && _selection > 0) {
      _selection--;
    }
    if (lib().is_key_pressed(Lib::KEY_DOWN) && _selection < 2) {
      _selection++;
    }
    if (t != _selection) {
      lib().play_sound(Lib::SOUND_MENU_CLICK);
    }

    if (lib().is_key_pressed(Lib::KEY_ACCEPT) ||
        lib().is_key_pressed(Lib::KEY_MENU)) {
      if (_selection == 0) {
        _state = STATE_GAME;
      }
      else if (_selection == 1) {
        _state = STATE_HIGHSCORE;
        _enter_char = 0;
        _compliment = z::rand_int(_compliments.size());
        _kill_timer = 0;
      }
      else if (_selection == 2) {
        _settings.volume = std::min(fixed(100), 1 + _settings.volume);
        _settings.save();
        lib().set_volume(_settings.volume.to_int());
      }
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }
    if (lib().is_key_pressed(Lib::KEY_LEFT) && _selection == 2) {
      if (_settings.volume > 0) {
        _settings.volume -= 1;
        lib().set_volume(_settings.volume.to_int());
        _settings.save();
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().is_key_pressed(Lib::KEY_RIGHT) && _selection == 2) {
      if (_settings.volume < 100) {
        _settings.volume += 1;
        lib().set_volume(_settings.volume.to_int());
        _settings.save();
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
    }
    if (lib().is_key_pressed(Lib::KEY_CANCEL)) {
      _state = STATE_GAME;
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }
  }
  // In-game
  //------------------------------
  else if (_state == STATE_GAME) {
    Boss::_warnings.clear();
    lib().capture_mouse(true);

    lib().set_colour_cycle(
        _mode == HARD_MODE ? 128 :
        _mode == FAST_MODE ? 192 :
        _mode == WHAT_MODE ? (lib().get_colour_cycle() + 1) % 256 : 0);
    if (Player::replay.recording) {
      _frame_count = _mode == FAST_MODE ? 2 : 1;
    }

    int controllers = 0;
    for (int32_t i = 0; i < count_players(); i++) {
      controllers += lib().get_pad_type(i);
    }
    if (controllers < _controllers_connected &&
        !_controllers_dialog && Player::replay.recording) {
      _controllers_dialog = true;
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }
    _controllers_connected = controllers;

    if (_controllers_dialog) {
      if (lib().is_key_pressed(Lib::KEY_MENU) ||
          lib().is_key_pressed(Lib::KEY_ACCEPT)) {
        _controllers_dialog = false;
        _first_controllers_dialog = false;
        lib().play_sound(Lib::SOUND_MENU_ACCEPT);
        for (int32_t i = 0; i < count_players(); i++) {
          lib().rumble(i, 10);
        }
      }
      return false;
    }

    if (lib().is_key_pressed(Lib::KEY_MENU)) {
      _state = STATE_PAUSE;
      _selection = 0;
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }

    if (!Player::replay.recording) {
      if (lib().is_key_pressed(Lib::KEY_BOMB)) {
        _frame_count *= 2;
      }
      if (lib().is_key_pressed(Lib::KEY_FIRE) &&
          _frame_count > (_mode == FAST_MODE ? 2 : 1)) {
        _frame_count /= 2;
      }
    }

    Player::update_fire_timer();
    ChaserBoss::_hasCounted = false;
    std::stable_sort(_collisions.begin(), _collisions.end(), &sort_ships);
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
    for (std::size_t i = 0; i < Boss::_fireworks.size(); ++i) {
      if (Boss::_fireworks[i].first <= 0) {
        vec2 v = _ships[0]->shape().centre;
        _ships[0]->shape().centre = Boss::_fireworks[i].second.first;
        _ships[0]->explosion(0xffffffff);
        _ships[0]->explosion(Boss::_fireworks[i].second.second, 16);
        _ships[0]->explosion(0xffffffff, 24);
        _ships[0]->explosion(Boss::_fireworks[i].second.second, 32);
        _ships[0]->shape().centre = v;
        Boss::_fireworks.erase(Boss::_fireworks.begin() + i);
        --i;
      }
      else {
        --Boss::_fireworks[i].first;
      }
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

    if (!_kill_timer && ((killed_players() == _players && !get_lives()) ||
                        (_mode == BOSS_MODE &&
                         _overmind->get_killed_bosses() >= 6))) {
      _kill_timer = 100;
    }
    if (_kill_timer) {
      _kill_timer--;
      if (!_kill_timer) {
        _state = STATE_HIGHSCORE;
        _score_screen_timer = 0;
        _enter_char = 0;
        _compliment = z::rand_int(_compliments.size());
        lib().play_sound(Lib::SOUND_MENU_ACCEPT);
      }
    }
  }
  // Entering high-score
  //------------------------------
  else if (_state == STATE_HIGHSCORE) {
    ++_score_screen_timer;
    _frame_count = 1;
#ifdef PLATFORM_SCORE
    int64_t score = get_total_score();
    if (_mode == BOSS_MODE) {
      score =
          _overmind->get_killed_bosses() >= 6 &&
          _overmind->get_elapsed_time() != 0 ?
              std::max(_overmind->get_elapsed_time() - 600l * get_lives(), 1l) : 0l;
    }
    std::cout << Player::replay.seed << "\n" << _players << "\n" <<
        (_mode == BOSS_MODE) << "\n" << (_mode == HARD_MODE) << "\n" <<
        (_mode == FAST_MODE) << "\n" << (_mode == WHAT_MODE) << "\n" <<
        score << "\n" << std::flush;
    throw score_finished{};
#endif

    if (is_high_score()) {
      _enter_time++;
      std::string chars = ALLOWED_CHARS;
      if (lib().is_key_pressed(Lib::KEY_ACCEPT) &&
          _enter_name.length() < SaveData::MAX_NAME_LENGTH) {
        _enter_name += chars.substr(_enter_char, 1);
        _enter_time = 0;
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().is_key_pressed(Lib::KEY_CANCEL) && _enter_name.length() > 0) {
        _enter_name = _enter_name.substr(0, _enter_name.length() - 1);
        _enter_time = 0;
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().is_key_pressed(Lib::KEY_RIGHT)) {
        _enter_r = 0;
        _enter_char++;
        if (_enter_char >= int(chars.length())) {
          _enter_char -= int(chars.length());
        }
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().is_key_held(Lib::KEY_RIGHT)) {
        _enter_r++;
        _enter_time = 16;
        if (_enter_r % 5 == 0 && _enter_r > 5) {
          _enter_char++;
          if (_enter_char >= int(chars.length())) {
            _enter_char -= int(chars.length());
          }
          lib().play_sound(Lib::SOUND_MENU_CLICK);
        }
      }
      if (lib().is_key_pressed(Lib::KEY_LEFT)) {
        _enter_r = 0;
        _enter_char--;
        if (_enter_char < 0) {
          _enter_char += int(chars.length());
        }
        lib().play_sound(Lib::SOUND_MENU_CLICK);
      }
      if (lib().is_key_held(Lib::KEY_LEFT)) {
        _enter_r++;
        _enter_time = 16;
        if (_enter_r % 5 == 0 && _enter_r > 5) {
          _enter_char--;
          if (_enter_char < 0) {
            _enter_char += int(chars.length());
          }
          lib().play_sound(Lib::SOUND_MENU_CLICK);
        }
      }

      if (lib().is_key_pressed(Lib::KEY_MENU)) {
        lib().play_sound(Lib::SOUND_MENU_ACCEPT);
        if (_mode != BOSS_MODE) {
          int32_t index =
              _mode == WHAT_MODE ? 3 * Lib::PLAYERS + _players :
              _mode == FAST_MODE ? 2 * Lib::PLAYERS + _players :
              _mode == HARD_MODE ? Lib::PLAYERS + _players : _players - 1;

          HighScoreList& list = _save.high_scores[index];
          list.push_back(HighScore{_enter_name, get_total_score()});
          std::stable_sort(list.begin(), list.end(), &score_sort);
          list.erase(list.begin() + (list.size() - 1));

          Player::replay.end_recording(_enter_name, get_total_score());
          end_game();
        }
        else {
          int64_t score = _overmind->get_elapsed_time();
          score -= 600l * get_lives();
          if (score <= 0)
            score = 1;
          _save.high_scores[Lib::PLAYERS][_players - 1].name = _enter_name;
          _save.high_scores[Lib::PLAYERS][_players - 1].score = score;

          Player::replay.end_recording(_enter_name, score);
          end_game();
        }
      }
    }
    else if (lib().is_key_pressed(Lib::KEY_MENU)) {
      Player::replay.end_recording(
          "untitled",
          _mode == BOSS_MODE ?
              (_overmind->get_killed_bosses() >= 6 &&
               _overmind->get_elapsed_time() != 0 ?
                   std::max(_overmind->get_elapsed_time() -
                            600l * get_lives(), 1l)
               : 0l) : get_total_score());

      end_game();
      lib().play_sound(Lib::SOUND_MENU_ACCEPT);
    }
  }
  return false;
}

void z0Game::render() const
{
  if (_exit_timer > 0 &&
      !Player::replay.okay && !Player::replay.error.empty()) {
    int y = 2;
    std::string s = Player::replay.error;
    std::size_t i;
    while ((i = s.find_first_of("\n")) != std::string::npos) {
      std::string t = s.substr(0, i);
      s = s.substr(i + 1);
      lib().render_text(flvec2(2, float(y)), t, PANEL_TEXT);
      ++y;
    }
    lib().render_text(flvec2(2, float(y)), s, PANEL_TEXT);
    return;
  }

  _show_hp_bar = false;
  _fill_hp_bar = 0;
  if (!_first_controllers_dialog) {
    Stars::render(_lib);
    for (const auto& particle : _particles) {
      lib().render_rect(particle.position + flvec2(1, 1),
                       particle.position - flvec2(1, 1), particle.colour);
    }
    for (std::size_t i = _players; i < _ships.size(); ++i) {
      _ships[i]->render();
    }
    for (std::size_t i = 0;
         i < std::size_t(_players) && i < _ships.size(); ++i) {
      _ships[i]->render();
    }
  }

  // In-game
  //------------------------------
  if (_state == STATE_GAME) {
    if (_controllers_dialog) {
      render_panel(flvec2(3.f, 3.f), flvec2(32.f, 8.f + 2 * _players));
      lib().render_text(flvec2(4.f, 4.f), "CONTROLLERS FOUND", PANEL_TEXT);

      for (int32_t i = 0; i < _players; i++) {
        std::stringstream ss;
        ss << "PLAYER " << (i + 1) << ": ";
        lib().render_text(flvec2(4.f, 8.f + 2 * i), ss.str(), PANEL_TEXT);

        std::stringstream ss2;
        int pads = lib().get_pad_type(i);
        if (!Player::replay.recording) {
          ss2 << "REPLAY";
          pads = 1;
        }
        else {
          if (!pads) {
            ss2 << "NONE";
          }
          if (pads & Lib::PAD_GAMEPAD) {
            ss2 << "GAMEPAD";
            if (pads & Lib::PAD_KEYMOUSE) {
              ss2 << ", KB/MOUSE";
            }
          }
          else if (pads & Lib::PAD_KEYMOUSE) {
            ss2 << "MOUSE & KEYBOARD";
          }
        }

        lib().render_text(flvec2(14.f, 8.f + 2 * i), ss2.str(),
                          pads ? Player::player_colour(i) : PANEL_TEXT);
      }
      return;
    }

    std::stringstream ss;
    ss << _lives << " live(s)";
    if (_mode != BOSS_MODE && _overmind->get_timer() >= 0) {
      int t = int(0.5f + _overmind->get_timer() / 60);
      ss << " " << (t < 10 ? "0" : "") << t;
    }

    lib().render_text(
        flvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
               Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f),
        ss.str(), PANEL_TRAN);

    for (std::size_t i = 0; i < _ships.size() + Boss::_warnings.size(); ++i) {
      if (i < _ships.size() && !(_ships[i]->type() & Ship::SHIP_ENEMY)) {
        continue;
      }
      flvec2 v = to_float(
          i < _ships.size() ? _ships[i]->shape().centre :
                              Boss::_warnings[i - _ships.size()]);

      if (v.x < -4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(v.x + Lib::WIDTH, 0.f) / Lib::WIDTH);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().render_line(flvec2(0.f, v.y), flvec2(6, v.y - 3), a);
        lib().render_line(flvec2(6.f, v.y - 3), flvec2(6, v.y + 3), a);
        lib().render_line(flvec2(6.f, v.y + 3), flvec2(0, v.y), a);
      }
      if (v.x >= Lib::WIDTH + 4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(2 * Lib::WIDTH - v.x, 0.f) / Lib::WIDTH);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().render_line(flvec2(float(Lib::WIDTH), v.y),
                          flvec2(Lib::WIDTH - 6.f, v.y - 3), a);
        lib().render_line(flvec2(Lib::WIDTH - 6, v.y - 3),
                          flvec2(Lib::WIDTH - 6.f, v.y + 3), a);
        lib().render_line(flvec2(Lib::WIDTH - 6, v.y + 3),
                          flvec2(float(Lib::WIDTH), v.y), a);
      }
      if (v.y < -4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(v.y + Lib::HEIGHT, 0.f) / Lib::HEIGHT);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().render_line(flvec2(v.x, 0.f), flvec2(v.x - 3, 6.f), a);
        lib().render_line(flvec2(v.x - 3, 6.f), flvec2(v.x + 3, 6.f), a);
        lib().render_line(flvec2(v.x + 3, 6.f), flvec2(v.x, 0.f), a);
      }
      if (v.y >= Lib::HEIGHT + 4) {
        int a = int(.5f + float(0x1) + float(0x9) *
                    std::max(2 * Lib::HEIGHT - v.y, 0.f) / Lib::HEIGHT);
        a |= a << 4;
        a = (a << 8) | (a << 16) | (a << 24) | 0x66;
        lib().render_line(flvec2(v.x, float(Lib::HEIGHT)),
                          flvec2(v.x - 3, Lib::HEIGHT - 6.f), a);
        lib().render_line(flvec2(v.x - 3, Lib::HEIGHT - 6.f),
                          flvec2(v.x + 3, Lib::HEIGHT - 6.f), a);
        lib().render_line(flvec2(v.x + 3, Lib::HEIGHT - 6.f),
                          flvec2(v.x, float(Lib::HEIGHT)), a);
      }
    }
    if (_mode == BOSS_MODE) {
      std::stringstream sst;
      sst << convert_to_time(_overmind->get_elapsed_time());
      lib().render_text(
        flvec2(Lib::WIDTH / (2 * Lib::TEXT_WIDTH) -
               sst.str().length() - 1.f, 1.f),
        sst.str(), PANEL_TRAN);
    }

    if (_show_hp_bar) {
      int32_t x = _mode == BOSS_MODE ? 48 : 0;
      lib().render_rect(
          flvec2(x + Lib::WIDTH / 2 - 48.f, 16.f),
          flvec2(x + Lib::WIDTH / 2 + 48.f, 32.f),
          PANEL_TRAN, 2);

      lib().render_rect(
          flvec2(x + Lib::WIDTH / 2 - 44.f, 16.f + 4),
          flvec2(x + Lib::WIDTH / 2 - 44.f + 88.f * _fill_hp_bar, 32.f - 4),
          PANEL_TRAN);
    }

    if (!Player::replay.recording) {
      int32_t x = _mode == FAST_MODE ? _frame_count / 2 : _frame_count;
      std::stringstream ss;
      ss << x << "X " <<
          int32_t(100 * float(Player::replay_frame) /
          Player::replay.player_frames.size()) << "%";

      lib().render_text(
          flvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                 Lib::HEIGHT / Lib::TEXT_HEIGHT - 3.f),
          ss.str(), PANEL_TRAN);
    }

  }
  // Main menu screen
  //------------------------------
  else if (_state == STATE_MENU) {
    // Main menu
    //------------------------------
    render_panel(flvec2(3.f, 3.f), flvec2(19.f, 14.f));

    lib().render_text(flvec2(37.f - 16, 3.f), "coded by: SEiKEN", PANEL_TEXT);
    lib().render_text(flvec2(37.f - 16, 4.f), "stu@seiken.co.uk", PANEL_TEXT);
    lib().render_text(flvec2(37.f - 9, 6.f), "-testers-", PANEL_TEXT);
    lib().render_text(flvec2(37.f - 9, 7.f), "MATT BELL", PANEL_TEXT);
    lib().render_text(flvec2(37.f - 9, 8.f), "RUFUZZZZZ", PANEL_TEXT);
    lib().render_text(flvec2(37.f - 9, 9.f), "SHADOW1W2", PANEL_TEXT);

    std::string b = "BOSSES:  ";
    int bb = is_hard_mode_unlocked() ?
        _save.hard_mode_bosses_killed : _save.bosses_killed;
    if (bb & BOSS_1A) b += "X"; else b += "-";
    if (bb & BOSS_1B) b += "X"; else b += "-";
    if (bb & BOSS_1C) b += "X"; else b += "-";
    if (bb & BOSS_3A) b += "X"; else b += " ";
    if (bb & BOSS_2A) b += "X"; else b += "-";
    if (bb & BOSS_2B) b += "X"; else b += "-";
    if (bb & BOSS_2C) b += "X"; else b += "-";
    lib().render_text(flvec2(37.f - 16, 13.f), b, PANEL_TEXT);

    lib().render_text(flvec2(4.f, 4.f), "WiiSPACE", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 8.f), "START GAME", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 10.f), "PLAYERS", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 12.f), "EXiT", PANEL_TEXT);

    if (_special_selection == 0 && is_boss_mode_unlocked()) {
      lib().render_text(flvec2(6.f, 6.f), "BOSS MODE", PANEL_TEXT);
      if (is_hard_mode_unlocked() && _selection == -1) {
        lib().render_text(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_special_selection == 1 && is_hard_mode_unlocked()) {
      lib().render_text(flvec2(6.f, 6.f), "HARD MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().render_text(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
      if (is_fast_mode_unlocked() && _selection == -1) {
        lib().render_text(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_special_selection == 2 && is_fast_mode_unlocked()) {
      lib().render_text(flvec2(6.f, 6.f), "FAST MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().render_text(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
      if (is_what_mode_unlocked() && _selection == -1) {
        lib().render_text(flvec2(6.f, 6.f), "         >", PANEL_TRAN);
      }
    }
    if (_special_selection == 3 && is_what_mode_unlocked()) {
      lib().render_text(flvec2(6.f, 6.f), "W-HAT MODE", PANEL_TEXT);
      if (_selection == -1) {
        lib().render_text(flvec2(5.f, 6.f), "<", PANEL_TRAN);
      }
    }

    flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
               float((8 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
    flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
              float((9 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
    lib().render_rect(low, hi, PANEL_TEXT, 1);

    if (_players > 1 && _selection == 1) {
      lib().render_text(flvec2(5.f, 10.f), "<", PANEL_TRAN);
    }
    if (_players < 4 && _selection == 1) {
      lib().render_text(flvec2(14.f + _players, 10.f), ">", PANEL_TRAN);
    }
    for (int32_t i = 0; i < _players; ++i) {
      std::stringstream ss;
      ss << (i + 1);
      lib().render_text(flvec2(14.f + i, 10.f), ss.str(),
                        Player::player_colour(i));
    }

    // High score table
    //------------------------------
    render_panel(flvec2(3.f, 15.f), flvec2(37.f, 27.f));

    std::stringstream ss;
    ss << _players;
    std::string s = "HiGH SCORES    ";
    s += _selection == -1 ?
        (_special_selection == 0 ? "BOSS MODE" :
         _special_selection == 1 ? "HARD MODE (" + ss.str() + "P)" :
         _special_selection == 2 ? "FAST MODE (" + ss.str() + "P)" :
         "W-HAT MODE (" + ss.str() + "P)") :
        _players == 1 ? "ONE PLAYER" :
        _players == 2 ? "TWO PLAYERS" :
        _players == 3 ? "THREE PLAYERS" :
        _players == 4 ? "FOUR PLAYERS" : "";
    lib().render_text(flvec2(4.f, 16.f), s, PANEL_TEXT);

    if (_selection == -1 && _special_selection == 0) {
      lib().render_text(flvec2(4.f, 18.f), "ONE PLAYER", PANEL_TEXT);
      lib().render_text(flvec2(4.f, 20.f), "TWO PLAYERS", PANEL_TEXT);
      lib().render_text(flvec2(4.f, 22.f), "THREE PLAYERS", PANEL_TEXT);
      lib().render_text(flvec2(4.f, 24.f), "FOUR PLAYERS", PANEL_TEXT);

      for (std::size_t i = 0; i < Lib::PLAYERS; i++) {
        std::string score =
            convert_to_time(_save.high_scores[Lib::PLAYERS][i].score);
        if (score.length() > SaveData::MAX_SCORE_LENGTH) {
          score = score.substr(0, SaveData::MAX_SCORE_LENGTH);
        }
        std::string name = _save.high_scores[Lib::PLAYERS][i].name;
        if (name.length() > SaveData::MAX_NAME_LENGTH) {
          name = name.substr(0, SaveData::MAX_NAME_LENGTH);
        }

        lib().render_text(flvec2(19.f, 18.f + i * 2), score, PANEL_TEXT);
        lib().render_text(flvec2(19.f, 19.f + i * 2), name, PANEL_TEXT);
      }
    }
    else {
      for (std::size_t i = 0; i < SaveData::NUM_HIGH_SCORES; i++) {
        std::stringstream ssi;
        ssi << (i + 1) << ".";
        lib().render_text(flvec2(4.f, 18.f + i), ssi.str(), PANEL_TEXT);

        int n = _selection != -1 ? _players - 1 :
            _special_selection * Lib::PLAYERS + _players;

        if (_save.high_scores[n][i].score <= 0) {
          continue;
        }

        std::stringstream ss;
        ss << _save.high_scores[n][i].score;
        std::string score = ss.str();
        if (score.length() > SaveData::MAX_SCORE_LENGTH) {
          score = score.substr(0, SaveData::MAX_SCORE_LENGTH);
        }
        std::string name = _save.high_scores[n][i].name;
        if (name.length() > SaveData::MAX_NAME_LENGTH) {
          name = name.substr(0, SaveData::MAX_NAME_LENGTH);
        }

        lib().render_text(flvec2(7.f, 18.f + i), score, PANEL_TEXT);
        lib().render_text(flvec2(19.f, 18.f + i), name, PANEL_TEXT);
      }
    }
  }
  // Pause menu
  //------------------------------
  else if (_state == STATE_PAUSE) {
    render_panel(flvec2(3.f, 3.f), flvec2(15.f, 14.f));

    lib().render_text(flvec2(4.f, 4.f), "PAUSED", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 8.f), "CONTINUE", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 10.f), "END GAME", PANEL_TEXT);
    lib().render_text(flvec2(6.f, 12.f), "VOL.", PANEL_TEXT);
    std::stringstream vol;
    int32_t v = _settings.volume.to_int();
    vol << " " << (v < 10 ? " " : "") << v;
    lib().render_text(flvec2(10.f, 12.f), vol.str(), PANEL_TEXT);
    if (_selection == 2 && v > 0) {
      lib().render_text(flvec2(5.f, 12.f), "<", PANEL_TRAN);
    }
    if (_selection == 2 && v < 100) {
      lib().render_text(flvec2(13.f, 12.f), ">", PANEL_TRAN);
    }

    flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
               float((8 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
    flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
              float((9 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
    lib().render_rect(low, hi, PANEL_TEXT, 1);
  }
  // Score screen
  //------------------------------
  else if (_state == STATE_HIGHSCORE) {
    // Name enter
    //------------------------------
    if (is_high_score()) {
      std::string chars = ALLOWED_CHARS;

      render_panel(flvec2(3.f, 20.f), flvec2(28.f, 27.f));
      lib().render_text(flvec2(4.f, 21.f), "It's a high score!", PANEL_TEXT);
      lib().render_text(flvec2(4.f, 23.f),
                       _players == 1 ? "Enter name:" : "Enter team name:",
                       PANEL_TEXT);
      lib().render_text(flvec2(6.f, 25.f), _enter_name, PANEL_TEXT);
      if ((_enter_time / 16) % 2 &&
          _enter_name.length() < SaveData::MAX_NAME_LENGTH) {
        lib().render_text(flvec2(6.f + _enter_name.length(), 25.f),
                          chars.substr(_enter_char, 1), 0xbbbbbbff);
      }
      flvec2 low(float(4 * Lib::TEXT_WIDTH + 4),
                 float((25 + 2 * _selection) * Lib::TEXT_HEIGHT + 4));
      flvec2 hi(float(5 * Lib::TEXT_WIDTH - 4),
                float((26 + 2 * _selection) * Lib::TEXT_HEIGHT - 4));
      lib().render_rect(low, hi, PANEL_TEXT, 1);
    }

    // Boss mode score listing
    //------------------------------
    if (_mode == BOSS_MODE) {
      int32_t extra_lives = get_lives();
      bool b = extra_lives > 0 && _overmind->get_killed_bosses() >= 6;

      long score = _overmind->get_elapsed_time();
      if (b) {
        score -= 10 * extra_lives;
      }
      if (score <= 0) {
        score = 1;
      }

      render_panel(flvec2(3.f, 3.f), flvec2(37.f, b ? 10.f : 8.f));
      if (b) {
        std::stringstream ss;
        ss << (extra_lives * 10) << "-second extra-life bonus!";
        lib().render_text(flvec2(4.f, 4.f), ss.str(), PANEL_TEXT);
      }

      lib().render_text(flvec2(4.f, b ? 6.f : 4.f),
                        "TIME ELAPSED: " + convert_to_time(score), PANEL_TEXT);
      std::stringstream ss;
      ss << "BOSS DESTROY: " << _overmind->get_killed_bosses();
      lib().render_text(flvec2(4.f, b ? 8.f : 6.f), ss.str(), PANEL_TEXT);
      return;
    }

    // Score listing
    //------------------------------
    render_panel(flvec2(3.f, 3.f),
                 flvec2(37.f, 8.f + 2 * _players + (_players > 1 ? 2 : 0)));

    std::stringstream ss;
    ss << get_total_score();
    std::string score = ss.str();
    if (score.length() > SaveData::MAX_SCORE_LENGTH) {
      score = score.substr(0, SaveData::MAX_SCORE_LENGTH);
    }
    lib().render_text(flvec2(4.f, 4.f), "TOTAL SCORE: " + score, PANEL_TEXT);

    for (int32_t i = 0; i < _players; ++i) {
      std::stringstream sss;
      if (_score_screen_timer % 600 < 300) {
        sss << get_player_score(i);
      }
      else {
        sss << get_player_deaths(i) << " death" <<
            (get_player_deaths(i) != 1 ? "s" : "");
      }
      score = sss.str();
      if (score.length() > SaveData::MAX_SCORE_LENGTH) {
        score = score.substr(0, SaveData::MAX_SCORE_LENGTH);
      }

      std::stringstream ssp;
      ssp << "PLAYER " << (i + 1) << ":";
      lib().render_text(flvec2(4.f, 8.f + 2 * i), ssp.str(), PANEL_TEXT);
      lib().render_text(flvec2(14.f, 8.f + 2 * i), score,
                        Player::player_colour(i));
    }

    // Top-ranked player
    //------------------------------
    if (_players > 1) {
      bool first = true;
      int64_t max = 0;
      std::size_t best = 0;
      for (int32_t i = 0; i < _players; ++i) {
        if (first || get_player_score(i) > max) {
          max = get_player_score(i);
          best = i;
        }
        first = false;
      }

      if (get_total_score() > 0) {
        std::stringstream s;
        s << "PLAYER " << (best + 1);
        lib().render_text(flvec2(4.f, 8.f + 2 * _players), s.str(),
                          Player::player_colour(best));

        std::string compliment = _compliments[_compliment];
        lib().render_text(
            flvec2(12.f, 8.f + 2 * _players), compliment, PANEL_TEXT);
      }
      else {
        lib().render_text(
            flvec2(4.f, 8.f + 2 * _players), "Oh dear!", PANEL_TEXT);
      }
    }
  }
}

// Game control
//------------------------------
void z0Game::new_game(bool can_face_secret_boss, bool replay, game_mode mode)
{
  if (!replay) {
    Player::replay = Replay(_players, mode, can_face_secret_boss);
    lib().new_game();
  }
  _controllers_dialog = true;
  _first_controllers_dialog = true;
  _controllers_connected = 0;
  _mode = mode;
  _overmind->reset(can_face_secret_boss);
  _lives = mode == BOSS_MODE ? _players * BOSSMODE_LIVES : STARTING_LIVES;
  _frame_count = _mode == FAST_MODE ? 2 : 1;

  Stars::clear();
  for (int32_t i = 0; i < _players; ++i) {
    vec2 v((1 + i) * Lib::WIDTH / (1 + _players), Lib::HEIGHT / 2);
    Player* p = new Player(v, i);
    add_ship(p);
    _player_list.push_back(p);
  }
  _state = STATE_GAME;
}

void z0Game::end_game()
{
  for (const auto& ship : _ships) {
    if (ship->type() & Ship::SHIP_ENEMY) {
      _overmind->on_enemy_destroy(*ship);
    }
  }

  Stars::clear();
  _ships.clear();
  _particles.clear();
  _player_list.clear();
  _collisions.clear();
  Boss::_fireworks.clear();
  Boss::_warnings.clear();

  _save.save();
  _frame_count = 1;
  _state = STATE_MENU;
  _selection =
      (_mode == BOSS_MODE && is_boss_mode_unlocked()) ||
      (_mode == HARD_MODE && is_hard_mode_unlocked()) ||
      (_mode == FAST_MODE && is_fast_mode_unlocked()) ||
      (_mode == WHAT_MODE && is_what_mode_unlocked()) ? -1 : 0;
  _special_selection =
      (_mode == BOSS_MODE && is_boss_mode_unlocked()) ? 0 :
      (_mode == HARD_MODE && is_hard_mode_unlocked()) ? 1 :
      (_mode == FAST_MODE && is_fast_mode_unlocked()) ? 2 :
      (_mode == WHAT_MODE && is_what_mode_unlocked()) ? 3 : 0;
}

void z0Game::add_ship(Ship* ship)
{
  ship->set_game(*this);
  if (ship->type() & Ship::SHIP_ENEMY) {
    _overmind->on_enemy_create(*ship);
  }
  _ships.emplace_back(ship);

  if (ship->bounding_width() > 1) {
    _collisions.push_back(ship);
  }
}

void z0Game::add_particle(const Particle& particle)
{
  _particles.emplace_back(particle);
}

// Ship info
//------------------------------
int z0Game::get_non_wall_count() const
{
  return _overmind->count_non_wall_enemies();
}

z0Game::ShipList z0Game::all_ships(int32_t ship_mask) const
{
  ShipList r;
  for (auto& ship : _ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

z0Game::ShipList z0Game::ships_in_radius(const vec2& point, fixed radius,
                                         int32_t ship_mask) const
{
  ShipList r;
  for (auto& ship : _ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->shape().centre - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool z0Game::any_collision(const vec2& point, int32_t category) const
{
  fixed x = point.x;
  fixed y = point.y;

  for (unsigned int i = 0; i < _collisions.size(); i++) {
    fixed sx = _collisions[i]->shape().centre.x;
    fixed sy = _collisions[i]->shape().centre.y;
    fixed w = _collisions[i]->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (_collisions[i]->check_point(point, category)) {
      return true;
    }
  }
  return false;
}

z0Game::ShipList z0Game::collision_list(const vec2& point, int32_t category) const
{
  ShipList r;
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

int32_t z0Game::alive_players() const
{
  return count_players() - killed_players();
}

int32_t z0Game::killed_players() const
{
  return Player::killed_players();
}

Player* z0Game::nearest_player(const vec2& point) const
{
  Ship* r = 0;
  Ship* deadr = 0;
  fixed d = Lib::WIDTH * Lib::HEIGHT;
  fixed deadd = Lib::WIDTH * Lib::HEIGHT;

  for (Ship* ship : _player_list) {
    if (!((Player*) ship)->is_killed() &&
        (ship->shape().centre - point).length() < d) {
      d = (ship->shape().centre - point).length();
      r = ship;
    }
    if ((ship->shape().centre - point).length() < deadd) {
      deadd = (ship->shape().centre - point).length();
      deadr = ship;
    }
  }
  return (Player*) (r != 0 ? r : deadr);
}

void z0Game::set_boss_killed(boss_list boss)
{
  if (!Player::replay.recording) {
    return;
  }
  if (boss == BOSS_3A || (_mode != BOSS_MODE && _mode != NORMAL_MODE)) {
    _save.hard_mode_bosses_killed |= boss;
  }
  else {
    _save.bosses_killed |= boss;
  }
}

bool z0Game::sort_ships(Ship* const& a, Ship* const& b)
{
  return a->shape().centre.x - a->bounding_width() <
         b->shape().centre.x - b->bounding_width();
}

// UI layout
//------------------------------
void z0Game::render_panel(const flvec2& low, const flvec2& hi) const
{
  flvec2 tlow(low.x * Lib::TEXT_WIDTH, low.y * Lib::TEXT_HEIGHT);
  flvec2 thi(hi.x * Lib::TEXT_WIDTH, hi.y * Lib::TEXT_HEIGHT);
  lib().render_rect(tlow, thi, PANEL_BACK);
  lib().render_rect(tlow, thi, PANEL_TEXT, 4);
}

std::string z0Game::convert_to_time(int64_t score) const
{
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

// Score calculation
//------------------------------
int64_t z0Game::get_player_score(int32_t player_number) const
{
  for (Ship* ship : _player_list) {
    Player* p = (Player*) ship;
    if (p->player_number() == player_number) {
      return p->score();
    }
  }
  return 0;
}

int64_t z0Game::get_player_deaths(int32_t player_number) const
{
  for (Ship* ship : _player_list) {
    Player* p = (Player*) ship;
    if (p->player_number() == player_number) {
      return p->deaths();
    }
  }
  return 0;
}

int64_t z0Game::get_total_score() const
{
  int64_t total = 0;
  for (Ship* ship : _player_list) {
    Player* p = (Player*) ship;
    total += p->score();
  }
  return total;
}

bool z0Game::is_high_score() const
{
  if (!Player::replay.recording) {
    return false;
  }

  if (_mode == BOSS_MODE) {
    return _overmind->get_killed_bosses() >= 6 &&
           _overmind->get_elapsed_time() != 0 &&
          (_overmind->get_elapsed_time() <
           _save.high_scores[Lib::PLAYERS][_players - 1].score ||
           _save.high_scores[Lib::PLAYERS][_players - 1].score == 0);
  }

  int n =
      _mode == WHAT_MODE ? 3 * Lib::PLAYERS + _players :
      _mode == FAST_MODE ? 2 * Lib::PLAYERS + _players :
      _mode == HARD_MODE ? Lib::PLAYERS + _players : _players - 1;

  const HighScoreList& list = _save.high_scores[n];
  for (unsigned int i = 0; i < list.size(); i++) {
    if (get_total_score() > list[i].score) {
      return true;
    }
  }
  return false;
}