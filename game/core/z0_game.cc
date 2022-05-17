#include "game/core/z0_game.h"
#include "game/core/lib.h"
#include "game/logic/player.h"
#include "game/logic/sim_state.h"
#include <algorithm>
#include <iostream>

namespace {
// Compliments have a max length of 24.
const std::vector<std::string> kCompliments{" is a swell guy!",         " went absolutely mental!",
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

const std::string kAllowedChars = "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! ";

std::string convert_to_time(std::int64_t score) {
  if (score == 0) {
    return "--:--";
  }
  std::int64_t mins = 0;
  while (score >= 60 * 60 && mins < 99) {
    score -= 60 * 60;
    ++mins;
  }
  std::int64_t secs = score / 60;

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
  fvec2 tlow(low.x * Lib::kTextWidth, low.y * Lib::kTextHeight);
  fvec2 thi(hi.x * Lib::kTextWidth, hi.y * Lib::kTextHeight);
  lib.render_rect(tlow, thi, z0Game::kPanelBack);
  lib.render_rect(tlow, thi, z0Game::kPanelText, 4);
}

}  // namespace

PauseModal::PauseModal(output_t* output, Settings& settings)
: Modal(true, false), _output(output), _settings(settings), _selection(0) {
  *output = kContinue;
}

void PauseModal::update(Lib& lib) {
  std::int32_t t = _selection;
  if (lib.is_key_pressed(Lib::key::kUp)) {
    _selection = std::max(0, _selection - 1);
  }
  if (lib.is_key_pressed(Lib::key::kDown)) {
    _selection = std::min(2, _selection + 1);
  }
  if (t != _selection) {
    lib.play_sound(Lib::sound::kMenuClick);
  }

  bool accept = lib.is_key_pressed(Lib::key::kAccept) || lib.is_key_pressed(Lib::key::kMenu);
  if (accept && _selection == 0) {
    quit();
  } else if (accept && _selection == 1) {
    *_output = kEndGame;
    quit();
  } else if (accept && _selection == 2) {
    _settings.volume = std::min(fixed(100), 1 + _settings.volume);
    _settings.save();
    lib.set_volume(_settings.volume.to_int());
  }
  if (accept) {
    lib.play_sound(Lib::sound::kMenuAccept);
  }

  if (lib.is_key_pressed(Lib::key::kCancel)) {
    quit();
    lib.play_sound(Lib::sound::kMenuAccept);
  }

  fixed v = _settings.volume;
  if (_selection == 2 && lib.is_key_pressed(Lib::key::kLeft)) {
    _settings.volume = std::max(fixed(0), _settings.volume - 1);
  }
  if (_selection == 2 && lib.is_key_pressed(Lib::key::kRight)) {
    _settings.volume = std::min(fixed(100), _settings.volume + 1);
  }
  if (v != _settings.volume) {
    lib.set_volume(_settings.volume.to_int());
    _settings.save();
    lib.play_sound(Lib::sound::kMenuClick);
  }
}

void PauseModal::render(Lib& lib) const {
  render_panel(lib, fvec2(3.f, 3.f), fvec2(15.f, 14.f));

  lib.render_text(fvec2(4.f, 4.f), "PAUSED", z0Game::kPanelText);
  lib.render_text(fvec2(6.f, 8.f), "kContinue", z0Game::kPanelText);
  lib.render_text(fvec2(6.f, 10.f), "END GAME", z0Game::kPanelText);
  lib.render_text(fvec2(6.f, 12.f), "VOL.", z0Game::kPanelText);
  std::stringstream vol;
  std::int32_t v = _settings.volume.to_int();
  vol << " " << (v < 10 ? " " : "") << v;
  lib.render_text(fvec2(10.f, 12.f), vol.str(), z0Game::kPanelText);
  if (_selection == 2 && v > 0) {
    lib.render_text(fvec2(5.f, 12.f), "<", z0Game::kPanelTran);
  }
  if (_selection == 2 && v < 100) {
    lib.render_text(fvec2(13.f, 12.f), ">", z0Game::kPanelTran);
  }

  fvec2 low(float(4 * Lib::kTextWidth + 4), float((8 + 2 * _selection) * Lib::kTextHeight + 4));
  fvec2 hi(float(5 * Lib::kTextWidth - 4), float((9 + 2 * _selection) * Lib::kTextHeight - 4));
  lib.render_rect(low, hi, z0Game::kPanelText, 1);
}

HighScoreModal::HighScoreModal(SaveData& save, GameModal& game, const SimState::results& results)
: Modal(true, false)
, _save(save)
, _game(game)
, _results(results)
, _enter_char(0)
, _enter_r(0)
, _enter_time(0)
, _compliment(z::rand_int(kCompliments.size()))
, _timer(0) {}

void HighScoreModal::update(Lib& lib) {
  ++_timer;

#ifdef PLATFORM_SCORE
  std::cout << _results.seed << "\n"
            << _results.players.size() << "\n"
            << (_results.mode == game_mode::kBoss) << "\n"
            << (_results.mode == game_mode::kHard) << "\n"
            << (_results.mode == game_mode::kFast) << "\n"
            << (_results.mode == game_mode::kWhat) << "\n"
            << get_score() << "\n"
            << std::flush;
  throw score_finished{};
#endif

  if (!is_high_score()) {
    if (lib.is_key_pressed(Lib::key::kMenu)) {
      _game.sim_state().write_replay("untitled", get_score());
      _save.save();
      lib.play_sound(Lib::sound::kMenuAccept);
      quit();
    }
    return;
  }

  _enter_time++;
  if (lib.is_key_pressed(Lib::key::kAccept) && _enter_name.length() < HighScores::kMaxNameLength) {
    _enter_name += kAllowedChars.substr(_enter_char, 1);
    _enter_time = 0;
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kCancel) && _enter_name.length() > 0) {
    _enter_name = _enter_name.substr(0, _enter_name.length() - 1);
    _enter_time = 0;
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kRight) || lib.is_key_pressed(Lib::key::kLeft)) {
    _enter_r = 0;
  }
  if (lib.is_key_held(Lib::key::kRight) || lib.is_key_held(Lib::key::kLeft)) {
    ++_enter_r;
    _enter_time = 16;
  }
  if (lib.is_key_pressed(Lib::key::kRight) ||
      (lib.is_key_held(Lib::key::kRight) && _enter_r % 5 == 0 && _enter_r > 5)) {
    _enter_char = (_enter_char + 1) % kAllowedChars.length();
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kLeft) ||
      (lib.is_key_held(Lib::key::kLeft) && _enter_r % 5 == 0 && _enter_r > 5)) {
    _enter_char = (_enter_char + kAllowedChars.length() - 1) % kAllowedChars.length();
    lib.play_sound(Lib::sound::kMenuClick);
  }

  if (lib.is_key_pressed(Lib::key::kMenu)) {
    lib.play_sound(Lib::sound::kMenuAccept);
    _save.high_scores.add_score(_results.mode, _results.players.size() - 1, _enter_name,
                                get_score());
    _game.sim_state().write_replay(_enter_name, get_score());
    _save.save();
    quit();
  }
}

void HighScoreModal::render(Lib& lib) const {
  std::int32_t players = _results.players.size();
  if (is_high_score()) {
    render_panel(lib, fvec2(3.f, 20.f), fvec2(28.f, 27.f));
    lib.render_text(fvec2(4.f, 21.f), "It's a high score!", z0Game::kPanelText);
    lib.render_text(fvec2(4.f, 23.f),
                    players == 1 ? "Enter name:" : "Enter team name:", z0Game::kPanelText);
    lib.render_text(fvec2(6.f, 25.f), _enter_name, z0Game::kPanelText);
    if ((_enter_time / 16) % 2 && _enter_name.length() < HighScores::kMaxNameLength) {
      lib.render_text(fvec2(6.f + _enter_name.length(), 25.f), kAllowedChars.substr(_enter_char, 1),
                      0xbbbbbbff);
    }
    fvec2 low(float(4 * Lib::kTextWidth + 4), float(25 * Lib::kTextHeight + 4));
    fvec2 hi(float(5 * Lib::kTextWidth - 4), float(26 * Lib::kTextHeight - 4));
    lib.render_rect(low, hi, z0Game::kPanelText, 1);
  }

  if (_results.mode == game_mode::kBoss) {
    std::int32_t extra_lives = _results.lives_remaining;
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
      lib.render_text(fvec2(4.f, 4.f), ss.str(), z0Game::kPanelText);
    }

    lib.render_text(fvec2(4.f, b ? 6.f : 4.f), "TIME ELAPSED: " + convert_to_time(score),
                    z0Game::kPanelText);
    std::stringstream ss;
    ss << "BOSS DESTROY: " << _results.killed_bosses;
    lib.render_text(fvec2(4.f, b ? 8.f : 6.f), ss.str(), z0Game::kPanelText);
    return;
  }

  render_panel(lib, fvec2(3.f, 3.f), fvec2(37.f, 8.f + 2 * players + (players > 1 ? 2 : 0)));

  std::stringstream ss;
  ss << get_score();
  std::string score = ss.str();
  if (score.length() > HighScores::kMaxScoreLength) {
    score = score.substr(0, HighScores::kMaxScoreLength);
  }
  lib.render_text(fvec2(4.f, 4.f), "TOTAL SCORE: " + score, z0Game::kPanelText);

  for (std::int32_t i = 0; i < players; ++i) {
    std::stringstream sss;
    if (_timer % 600 < 300) {
      sss << _results.players[i].score;
    } else {
      auto deaths = _results.players[i].deaths;
      sss << deaths << " death" << (deaths != 1 ? "s" : "");
    }
    score = sss.str();
    if (score.length() > HighScores::kMaxScoreLength) {
      score = score.substr(0, HighScores::kMaxScoreLength);
    }

    std::stringstream ssp;
    ssp << "PLAYER " << (i + 1) << ":";
    lib.render_text(fvec2(4.f, 8.f + 2 * i), ssp.str(), z0Game::kPanelText);
    lib.render_text(fvec2(14.f, 8.f + 2 * i), score, Player::player_colour(i));
  }

  if (players <= 1) {
    return;
  }

  bool first = true;
  std::int64_t max = 0;
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

    std::string compliment = kCompliments[_compliment];
    lib.render_text(fvec2(12.f, 8.f + 2 * players), compliment, z0Game::kPanelText);
  } else {
    lib.render_text(fvec2(4.f, 8.f + 2 * players), "Oh dear!", z0Game::kPanelText);
  }
}

std::int64_t HighScoreModal::get_score() const {
  if (_results.mode == game_mode::kBoss) {
    bool won = _results.killed_bosses >= 6 && _results.elapsed_time != 0;
    return !won ? 0l : std::max(1l, _results.elapsed_time - 600l * _results.lives_remaining);
  }
  std::int64_t total = 0;
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
                     game_mode mode, std::int32_t player_count, bool can_face_secret_boss)
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
  if (_pause_output == PauseModal::kEndGame || _state->game_over()) {
    add(new HighScoreModal(_save, *this, _state->get_results()));
    *_frame_count = 1;
    if (_pause_output != PauseModal::kEndGame) {
      lib.play_sound(Lib::sound::kMenuAccept);
    }
    quit();
    return;
  }

  if (lib.is_key_pressed(Lib::key::kMenu)) {
    lib.capture_mouse(false);
    add(new PauseModal(&_pause_output, _settings));
    lib.play_sound(Lib::sound::kMenuAccept);
    return;
  }
  lib.capture_mouse(true);

  auto results = _state->get_results();
  std::int32_t controllers = 0;
  for (std::size_t i = 0; i < results.players.size(); ++i) {
    controllers |= lib.get_pad_type(i);
  }
  if (controllers < _controllers_connected && !_controllers_dialog && !results.is_replay) {
    _controllers_dialog = true;
    lib.play_sound(Lib::sound::kMenuAccept);
  }
  _controllers_connected = controllers;

  if (_controllers_dialog) {
    if (lib.is_key_pressed(Lib::key::kMenu) || lib.is_key_pressed(Lib::key::kAccept)) {
      _controllers_dialog = false;
      lib.play_sound(Lib::sound::kMenuAccept);
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
    lib.render_text(fvec2(4.f, 4.f), "CONTROLLERS FOUND", z0Game::kPanelText);

    for (std::size_t i = 0; i < results.players.size(); i++) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      lib.render_text(fvec2(4.f, 8.f + 2 * i), ss.str(), z0Game::kPanelText);

      std::stringstream ss2;
      std::int32_t pads = lib.get_pad_type(i);
      if (results.is_replay) {
        ss2 << "REPLAY";
        pads = 1;
      } else {
        if (!pads) {
          ss2 << "NONE";
        }
        if (pads & Lib::pad_type::kPadGamepad) {
          ss2 << "GAMEPAD";
          if (pads & Lib::pad_type::kPadKeyboardMouse) {
            ss2 << ", KB/MOUSE";
          }
        } else if (pads & Lib::pad_type::kPadKeyboardMouse) {
          ss2 << "MOUSE & KEYBOARD";
        }
      }

      lib.render_text(fvec2(14.f, 8.f + 2 * i), ss2.str(),
                      pads ? Player::player_colour(i) : z0Game::kPanelText);
    }
    return;
  }

  std::stringstream ss;
  ss << results.lives_remaining << " live(s)";
  if (results.mode != game_mode::kBoss && results.overmind_timer >= 0) {
    std::int32_t t = std::int32_t(0.5f + results.overmind_timer / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  lib.render_text(fvec2(Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                        Lib::kHeight / Lib::kTextHeight - 2.f),
                  ss.str(), z0Game::kPanelTran);

  if (results.mode == game_mode::kBoss) {
    std::stringstream sst;
    sst << convert_to_time(results.elapsed_time);
    lib.render_text(fvec2(Lib::kWidth / (2 * Lib::kTextWidth) - sst.str().length() - 1.f, 1.f),
                    sst.str(), z0Game::kPanelTran);
  }

  if (results.boss_hp_bar) {
    std::int32_t x = results.mode == game_mode::kBoss ? 48 : 0;
    lib.render_rect(fvec2(x + Lib::kWidth / 2 - 48.f, 16.f),
                    fvec2(x + Lib::kWidth / 2 + 48.f, 32.f), z0Game::kPanelTran, 2);

    lib.render_rect(fvec2(x + Lib::kWidth / 2 - 44.f, 16.f + 4),
                    fvec2(x + Lib::kWidth / 2 - 44.f + 88.f * *results.boss_hp_bar, 32.f - 4),
                    z0Game::kPanelTran);
  }

  if (results.is_replay) {
    std::int32_t x = results.mode == game_mode::kFast ? *_frame_count / 2 : *_frame_count;
    std::stringstream ss;
    ss << x << "X " << std::int32_t(100 * results.replay_progress) << "%";

    lib.render_text(fvec2(Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                          Lib::kHeight / Lib::kTextHeight - 3.f),
                    ss.str(), z0Game::kPanelTran);
  }
}

z0Game::z0Game(Lib& lib, const std::vector<std::string>& args)
: _lib(lib)
, _frame_count(1)
, _menu_select(menu::kStart)
, _player_select(1)
, _mode_select(game_mode::kBoss)
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

  for (std::int32_t i = 0; i < kPlayers; i++) {
    if (lib().is_key_held(i, Lib::key::kFire) && lib().is_key_held(i, Lib::key::kBomb) &&
        lib().is_key_pressed(Lib::key::kMenu)) {
      lib().take_screenshot();
      break;
    }
  }
  lib().capture_mouse(false);

  if (_modals.update(_lib)) {
    return false;
  }

  menu t = _menu_select;
  if (lib().is_key_pressed(Lib::key::kUp)) {
    _menu_select = static_cast<menu>(
        std::max(static_cast<std::int32_t>(_menu_select) - 1,
                 static_cast<std::int32_t>(mode_unlocked() >= game_mode::kBoss ? menu::kSpecial
                                                                               : menu::kStart)));
  }
  if (lib().is_key_pressed(Lib::key::kDown)) {
    _menu_select = static_cast<menu>(std::min(static_cast<std::int32_t>(menu::kQuit),
                                              static_cast<std::int32_t>(_menu_select) + 1));
  }
  if (t != _menu_select) {
    lib().play_sound(Lib::sound::kMenuClick);
  }

  if (_menu_select == menu::kPlayers) {
    std::int32_t t = _player_select;
    if (lib().is_key_pressed(Lib::key::kLeft)) {
      _player_select = std::max(1, _player_select - 1);
    }
    if (lib().is_key_pressed(Lib::key::kRight)) {
      _player_select = std::min(kPlayers, _player_select + 1);
    }
    if (lib().is_key_pressed(Lib::key::kAccept) || lib().is_key_pressed(Lib::key::kMenu)) {
      _player_select = 1 + _player_select % kPlayers;
    }
    if (t != _player_select) {
      lib().play_sound(Lib::sound::kMenuClick);
    }
    lib().set_player_count(_player_select);
  }

  if (_menu_select == menu::kSpecial) {
    game_mode t = _mode_select;
    if (lib().is_key_pressed(Lib::key::kLeft)) {
      _mode_select = static_cast<game_mode>(std::max(static_cast<std::int32_t>(game_mode::kBoss),
                                                     static_cast<std::int32_t>(_mode_select) - 1));
    }
    if (lib().is_key_pressed(Lib::key::kRight)) {
      _mode_select = static_cast<game_mode>(std::min(static_cast<std::int32_t>(mode_unlocked()),
                                                     static_cast<std::int32_t>(_mode_select) + 1));
    }
    if (t != _mode_select) {
      lib().play_sound(Lib::sound::kMenuClick);
    }
  }

  if (lib().is_key_pressed(Lib::key::kAccept) || lib().is_key_pressed(Lib::key::kMenu)) {
    if (_menu_select == menu::kStart) {
      lib().new_game();
      _modals.add(new GameModal(_lib, _save, _settings, &_frame_count, game_mode::kNormal,
                                _player_select, mode_unlocked() >= game_mode::kFast));
    } else if (_menu_select == menu::kQuit) {
      _exit_timer = 2;
    } else if (_menu_select == menu::kSpecial) {
      lib().new_game();
      _modals.add(new GameModal(_lib, _save, _settings, &_frame_count, _mode_select, _player_select,
                                mode_unlocked() >= game_mode::kFast));
    }
    if (_menu_select != menu::kPlayers) {
      lib().play_sound(Lib::sound::kMenuAccept);
    }
  }

  if (_menu_select >= menu::kStart || _mode_select == game_mode::kBoss) {
    lib().set_colour_cycle(0);
  } else if (_mode_select == game_mode::kHard) {
    lib().set_colour_cycle(128);
  } else if (_mode_select == game_mode::kFast) {
    lib().set_colour_cycle(192);
  } else if (_mode_select == game_mode::kWhat) {
    lib().set_colour_cycle((lib().get_colour_cycle() + 1) % 256);
  }
  return false;
}

void z0Game::render() const {
  if (_exit_timer > 0 && !_exit_error.empty()) {
    std::int32_t y = 2;
    std::string s = _exit_error;
    std::size_t i;
    while ((i = s.find_first_of("\n")) != std::string::npos) {
      std::string t = s.substr(0, i);
      s = s.substr(i + 1);
      lib().render_text(fvec2(2, float(y)), t, kPanelText);
      ++y;
    }
    lib().render_text(fvec2(2, float(y)), s, kPanelText);
    return;
  }

  if (_modals.render(_lib)) {
    return;
  }

  render_panel(lib(), fvec2(3.f, 3.f), fvec2(19.f, 14.f));

  lib().render_text(fvec2(37.f - 16, 3.f), "coded by: SEiKEN", kPanelText);
  lib().render_text(fvec2(37.f - 16, 4.f), "stu@seiken.co.uk", kPanelText);
  lib().render_text(fvec2(37.f - 9, 6.f), "-testers-", kPanelText);
  lib().render_text(fvec2(37.f - 9, 7.f), "MATT BELL", kPanelText);
  lib().render_text(fvec2(37.f - 9, 8.f), "RUFUZZZZZ", kPanelText);
  lib().render_text(fvec2(37.f - 9, 9.f), "SHADOW1W2", kPanelText);

  std::string b = "BOSSES:  ";
  std::int32_t bb =
      mode_unlocked() >= game_mode::kHard ? _save.hard_mode_bosses_killed : _save.bosses_killed;
  b += bb & SimState::BOSS_1A ? "X" : "-";
  b += bb & SimState::BOSS_1B ? "X" : "-";
  b += bb & SimState::BOSS_1C ? "X" : "-";
  b += bb & SimState::BOSS_3A ? "X" : " ";
  b += bb & SimState::BOSS_2A ? "X" : "-";
  b += bb & SimState::BOSS_2B ? "X" : "-";
  b += bb & SimState::BOSS_2C ? "X" : "-";
  lib().render_text(fvec2(37.f - 16, 13.f), b, kPanelText);

  lib().render_text(fvec2(4.f, 4.f), "WiiSPACE", kPanelText);
  lib().render_text(fvec2(6.f, 8.f), "START GAME", kPanelText);
  lib().render_text(fvec2(6.f, 10.f), "kPlayers", kPanelText);
  lib().render_text(fvec2(6.f, 12.f), "EXiT", kPanelText);

  if (mode_unlocked() >= game_mode::kBoss) {
    std::string str = _mode_select == game_mode::kBoss ? "BOSS MODE"
        : _mode_select == game_mode::kHard             ? "HARD MODE"
        : _mode_select == game_mode::kFast             ? "FAST MODE"
                                                       : "W-HAT MODE";
    lib().render_text(fvec2(6.f, 6.f), str, kPanelText);
  }
  if (_menu_select == menu::kSpecial && _mode_select > game_mode::kBoss) {
    lib().render_text(fvec2(5.f, 6.f), "<", kPanelTran);
  }
  if (_menu_select == menu::kSpecial && _mode_select < mode_unlocked()) {
    lib().render_text(fvec2(6.f, 6.f), "         >", kPanelTran);
  }

  fvec2 low(float(4 * Lib::kTextWidth + 4),
            float((6 + 2 * static_cast<std::int32_t>(_menu_select)) * Lib::kTextHeight + 4));
  fvec2 hi(float(5 * Lib::kTextWidth - 4),
           float((7 + 2 * static_cast<std::int32_t>(_menu_select)) * Lib::kTextHeight - 4));
  lib().render_rect(low, hi, kPanelText, 1);

  if (_player_select > 1 && _menu_select == menu::kPlayers) {
    lib().render_text(fvec2(5.f, 10.f), "<", kPanelTran);
  }
  if (_player_select < 4 && _menu_select == menu::kPlayers) {
    lib().render_text(fvec2(14.f + _player_select, 10.f), ">", kPanelTran);
  }
  for (std::int32_t i = 0; i < _player_select; ++i) {
    std::stringstream ss;
    ss << (i + 1);
    lib().render_text(fvec2(14.f + i, 10.f), ss.str(), Player::player_colour(i));
  }

  render_panel(lib(), fvec2(3.f, 15.f), fvec2(37.f, 27.f));

  std::stringstream ss;
  ss << _player_select;
  std::string s = "HiGH SCORES    ";
  s += _menu_select == menu::kSpecial
      ? (_mode_select == game_mode::kBoss       ? "BOSS MODE"
             : _mode_select == game_mode::kHard ? "HARD MODE (" + ss.str() + "P)"
             : _mode_select == game_mode::kFast ? "FAST MODE (" + ss.str() + "P)"
                                                : "W-HAT MODE (" + ss.str() + "P)")
      : _player_select == 1 ? "ONE PLAYER"
      : _player_select == 2 ? "TWO kPlayers"
      : _player_select == 3 ? "THREE kPlayers"
      : _player_select == 4 ? "FOUR kPlayers"
                            : "";
  lib().render_text(fvec2(4.f, 16.f), s, kPanelText);

  if (_menu_select == menu::kSpecial && _mode_select == game_mode::kBoss) {
    lib().render_text(fvec2(4.f, 18.f), "ONE PLAYER", kPanelText);
    lib().render_text(fvec2(4.f, 20.f), "TWO kPlayers", kPanelText);
    lib().render_text(fvec2(4.f, 22.f), "THREE kPlayers", kPanelText);
    lib().render_text(fvec2(4.f, 24.f), "FOUR kPlayers", kPanelText);

    for (std::size_t i = 0; i < kPlayers; i++) {
      auto& s = _save.high_scores.get(game_mode::kBoss, i, 0);
      std::string score = convert_to_time(s.score).substr(0, HighScores::kMaxNameLength);
      std::string name = s.name.substr(HighScores::kMaxNameLength);

      lib().render_text(fvec2(19.f, 18.f + i * 2), score, kPanelText);
      lib().render_text(fvec2(19.f, 19.f + i * 2), name, kPanelText);
    }
  } else {
    for (std::size_t i = 0; i < HighScores::kNumScores; i++) {
      std::stringstream ssi;
      ssi << (i + 1) << ".";
      lib().render_text(fvec2(4.f, 18.f + i), ssi.str(), kPanelText);

      auto& s =
          _save.high_scores.get(_menu_select == menu::kSpecial ? _mode_select : game_mode::kNormal,
                                _player_select - 1, i);
      if (s.score <= 0) {
        continue;
      }

      std::stringstream ss;
      ss << s.score;
      std::string score = ss.str().substr(0, HighScores::kMaxScoreLength);
      std::string name = s.name.substr(0, HighScores::kMaxScoreLength);

      lib().render_text(fvec2(7.f, 18.f + i), score, kPanelText);
      lib().render_text(fvec2(19.f, 18.f + i), name, kPanelText);
    }
  }
}

game_mode z0Game::mode_unlocked() const {
  if (!(_save.bosses_killed & 63)) {
    return game_mode::kNormal;
  }
  if (_save.high_scores.boss[0].score == 0 && _save.high_scores.boss[1].score == 0 &&
      _save.high_scores.boss[2].score == 0 && _save.high_scores.boss[3].score == 0) {
    return game_mode::kBoss;
  }
  if ((_save.hard_mode_bosses_killed & 63) != 63) {
    return game_mode::kHard;
  }
  if ((_save.hard_mode_bosses_killed & 64) != 64) {
    return game_mode::kFast;
  }
  return game_mode::kWhat;
}
