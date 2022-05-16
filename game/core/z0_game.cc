#include "game/core/z0_game.h"
#include "game/core/lib.h"
#include "game/logic/player.h"
#include "game/logic/sim_state.h"
#include <algorithm>
#include <iostream>

namespace {

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

void render_panel(Lib& lib, const fvec2& low, const fvec2& hi) {
  fvec2 tlow(low.x * Lib::TEXT_WIDTH, low.y * Lib::TEXT_HEIGHT);
  fvec2 thi(hi.x * Lib::TEXT_WIDTH, hi.y * Lib::TEXT_HEIGHT);
  lib.render_rect(tlow, thi, z0Game::PANEL_BACK);
  lib.render_rect(tlow, thi, z0Game::PANEL_TEXT, 4);
}

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

HighScoreModal::HighScoreModal(SaveData& save, GameModal& game, const SimState::results& results)
: Modal(true, false)
, _save(save)
, _game(game)
, _results(results)
, _enter_char(0)
, _enter_r(0)
, _enter_time(0)
, _compliment(z::rand_int(COMPLIMENTS.size()))
, _timer(0) {}

void HighScoreModal::update(Lib& lib) {
  ++_timer;

#ifdef PLATFORM_SCORE
  std::cout << _results.seed << "\n"
            << _results.players.size() << "\n"
            << (_results.mode == Mode::BOSS) << "\n"
            << (_results.mode == Mode::HARD) << "\n"
            << (_results.mode == Mode::FAST) << "\n"
            << (_results.mode == Mode::WHAT) << "\n"
            << get_score() << "\n"
            << std::flush;
  throw score_finished{};
#endif

  if (!is_high_score()) {
    if (lib.is_key_pressed(Lib::KEY_MENU)) {
      _game.sim_state().write_replay("untitled", get_score());
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
    _save.high_scores.add_score(_results.mode, _results.players.size() - 1, _enter_name,
                                get_score());
    _game.sim_state().write_replay(_enter_name, get_score());
    _save.save();
    quit();
  }
}

void HighScoreModal::render(Lib& lib) const {
  int32_t players = _results.players.size();
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

  if (_results.mode == Mode::BOSS) {
    int32_t extra_lives = _results.lives_remaining;
    bool b = extra_lives > 0 && _results.killed_bosses >= 6;

    long score = _results.elapsed_time;
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
    ss << "BOSS DESTROY: " << _results.killed_bosses;
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
      sss << _results.players[i].score;
    } else {
      auto deaths = _results.players[i].deaths;
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
  for (const auto& p : _results.players) {
    if (first || p.score > max) {
      max = p.score;
      best = p.number;
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
  if (_results.mode == Mode::BOSS) {
    bool won = _results.killed_bosses >= 6 && _results.elapsed_time != 0;
    return !won ? 0l : std::max(1l, _results.elapsed_time - 600l * _results.lives_remaining);
  }
  int64_t total = 0;
  for (const auto& p : _results.players) {
    total += p.score;
  }
  return total;
}

bool HighScoreModal::is_high_score() const {
  return !_results.is_replay &&
      _save.high_scores.is_high_score(_results.mode, _results.players.size() - 1, get_score());
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
                     Mode::mode mode, int32_t player_count, bool can_face_secret_boss)
: Modal(true, true), _save{save}, _settings{settings}, _frame_count{frame_count} {
  _state =
      std::make_unique<SimState>(lib, save, frame_count, mode, player_count, can_face_secret_boss);
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
                     const std::string& replay_path)
: Modal(true, true), _save{save}, _settings{settings}, _frame_count{frame_count} {
  _state = std::make_unique<SimState>(lib, save, frame_count, replay_path);
}

GameModal::~GameModal() {}

void GameModal::update(Lib& lib) {
  if (_pause_output == PauseModal::END_GAME || _state->game_over()) {
    add(new HighScoreModal(_save, *this, _state->get_results()));
    *_frame_count = 1;
    if (_pause_output != PauseModal::END_GAME) {
      lib.play_sound(Lib::SOUND_MENU_ACCEPT);
    }
    quit();
    return;
  }

  if (lib.is_key_pressed(Lib::KEY_MENU)) {
    lib.capture_mouse(false);
    add(new PauseModal(&_pause_output, _settings));
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
    return;
  }
  lib.capture_mouse(true);

  auto results = _state->get_results();
  int32_t controllers = 0;
  for (std::size_t i = 0; i < results.players.size(); ++i) {
    controllers += lib.get_pad_type(i);
  }
  if (controllers < _controllers_connected && !_controllers_dialog && !results.is_replay) {
    _controllers_dialog = true;
    lib.play_sound(Lib::SOUND_MENU_ACCEPT);
  }
  _controllers_connected = controllers;

  if (_controllers_dialog) {
    if (lib.is_key_pressed(Lib::KEY_MENU) || lib.is_key_pressed(Lib::KEY_ACCEPT)) {
      _controllers_dialog = false;
      lib.play_sound(Lib::SOUND_MENU_ACCEPT);
      for (std::size_t i = 0; i < results.players.size(); ++i) {
        lib.rumble(i, 10);
      }
    }
    return;
  }

  _state->update(lib);
}

void GameModal::render(Lib& lib) const {
  _state->render(lib);
  auto results = _state->get_results();

  if (_controllers_dialog) {
    render_panel(lib, fvec2(3.f, 3.f), fvec2(32.f, 8.f + 2 * results.players.size()));
    lib.render_text(fvec2(4.f, 4.f), "CONTROLLERS FOUND", z0Game::PANEL_TEXT);

    for (std::size_t i = 0; i < results.players.size(); i++) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      lib.render_text(fvec2(4.f, 8.f + 2 * i), ss.str(), z0Game::PANEL_TEXT);

      std::stringstream ss2;
      int32_t pads = lib.get_pad_type(i);
      if (results.is_replay) {
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
  ss << results.lives_remaining << " live(s)";
  if (results.mode != Mode::BOSS && results.overmind_timer >= 0) {
    int32_t t = int32_t(0.5f + results.overmind_timer / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  lib.render_text(fvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                        Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f),
                  ss.str(), z0Game::PANEL_TRAN);

  if (results.mode == Mode::BOSS) {
    std::stringstream sst;
    sst << convert_to_time(results.elapsed_time);
    lib.render_text(fvec2(Lib::WIDTH / (2 * Lib::TEXT_WIDTH) - sst.str().length() - 1.f, 1.f),
                    sst.str(), z0Game::PANEL_TRAN);
  }

  if (results.boss_hp_bar) {
    int32_t x = results.mode == Mode::BOSS ? 48 : 0;
    lib.render_rect(fvec2(x + Lib::WIDTH / 2 - 48.f, 16.f), fvec2(x + Lib::WIDTH / 2 + 48.f, 32.f),
                    z0Game::PANEL_TRAN, 2);

    lib.render_rect(fvec2(x + Lib::WIDTH / 2 - 44.f, 16.f + 4),
                    fvec2(x + Lib::WIDTH / 2 - 44.f + 88.f * *results.boss_hp_bar, 32.f - 4),
                    z0Game::PANEL_TRAN);
  }

  if (results.is_replay) {
    int32_t x = results.mode == Mode::FAST ? *_frame_count / 2 : *_frame_count;
    std::stringstream ss;
    ss << x << "X " << int32_t(100 * results.replay_progress) << "%";

    lib.render_text(fvec2(Lib::WIDTH / (2.f * Lib::TEXT_WIDTH) - ss.str().length() / 2,
                          Lib::HEIGHT / Lib::TEXT_HEIGHT - 3.f),
                    ss.str(), z0Game::PANEL_TRAN);
  }
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
  b += bb & SimState::BOSS_1A ? "X" : "-";
  b += bb & SimState::BOSS_1B ? "X" : "-";
  b += bb & SimState::BOSS_1C ? "X" : "-";
  b += bb & SimState::BOSS_3A ? "X" : " ";
  b += bb & SimState::BOSS_2A ? "X" : "-";
  b += bb & SimState::BOSS_2B ? "X" : "-";
  b += bb & SimState::BOSS_2C ? "X" : "-";
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
