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
  fvec2 tlow{low.x * Lib::kTextWidth, low.y * Lib::kTextHeight};
  fvec2 thi{hi.x * Lib::kTextWidth, hi.y * Lib::kTextHeight};
  lib.render_rect(tlow, thi, z0Game::kPanelBack);
  lib.render_rect(tlow, thi, z0Game::kPanelText, 4);
}

}  // namespace

PauseModal::PauseModal(output_t* output, Settings& settings)
: Modal{true, false}, output_{output}, settings_{settings} {
  *output = kContinue;
}

void PauseModal::update(Lib& lib) {
  std::int32_t t = selection_;
  if (lib.is_key_pressed(Lib::key::kUp)) {
    selection_ = std::max(0, selection_ - 1);
  }
  if (lib.is_key_pressed(Lib::key::kDown)) {
    selection_ = std::min(2, selection_ + 1);
  }
  if (t != selection_) {
    lib.play_sound(Lib::sound::kMenuClick);
  }

  bool accept = lib.is_key_pressed(Lib::key::kAccept) || lib.is_key_pressed(Lib::key::kMenu);
  if (accept && selection_ == 0) {
    quit();
  } else if (accept && selection_ == 1) {
    *output_ = kEndGame;
    quit();
  } else if (accept && selection_ == 2) {
    settings_.volume = std::min(fixed{100}, 1 + settings_.volume);
    settings_.save();
    lib.set_volume(settings_.volume.to_int());
  }
  if (accept) {
    lib.play_sound(Lib::sound::kMenuAccept);
  }

  if (lib.is_key_pressed(Lib::key::kCancel)) {
    quit();
    lib.play_sound(Lib::sound::kMenuAccept);
  }

  fixed v = settings_.volume;
  if (selection_ == 2 && lib.is_key_pressed(Lib::key::kLeft)) {
    settings_.volume = std::max(fixed{0}, settings_.volume - 1);
  }
  if (selection_ == 2 && lib.is_key_pressed(Lib::key::kRight)) {
    settings_.volume = std::min(fixed{100}, settings_.volume + 1);
  }
  if (v != settings_.volume) {
    lib.set_volume(settings_.volume.to_int());
    settings_.save();
    lib.play_sound(Lib::sound::kMenuClick);
  }
}

void PauseModal::render(Lib& lib) const {
  render_panel(lib, {3.f, 3.f}, {15.f, 14.f});

  lib.render_text({4.f, 4.f}, "PAUSED", z0Game::kPanelText);
  lib.render_text({6.f, 8.f}, "CONTINUE", z0Game::kPanelText);
  lib.render_text({6.f, 10.f}, "END GAME", z0Game::kPanelText);
  lib.render_text({6.f, 12.f}, "VOL.", z0Game::kPanelText);
  std::stringstream vol;
  std::int32_t v = settings_.volume.to_int();
  vol << " " << (v < 10 ? " " : "") << v;
  lib.render_text({10.f, 12.f}, vol.str(), z0Game::kPanelText);
  if (selection_ == 2 && v > 0) {
    lib.render_text({5.f, 12.f}, "<", z0Game::kPanelTran);
  }
  if (selection_ == 2 && v < 100) {
    lib.render_text({13.f, 12.f}, ">", z0Game::kPanelTran);
  }

  fvec2 low{static_cast<float>(4 * Lib::kTextWidth + 4),
            static_cast<float>((8 + 2 * selection_) * Lib::kTextHeight + 4)};
  fvec2 hi{static_cast<float>(5 * Lib::kTextWidth - 4),
           static_cast<float>((9 + 2 * selection_) * Lib::kTextHeight - 4)};
  lib.render_rect(low, hi, z0Game::kPanelText, 1);
}

HighScoreModal::HighScoreModal(SaveData& save, GameModal& game, const SimState::results& results)
: Modal{true, false}
, save_{save}
, game_{game}
, results_{results}
, compliment_{z::rand_int(kCompliments.size())} {}

void HighScoreModal::update(Lib& lib) {
  ++timer_;

#ifdef PLATFORM_SCORE
  std::cout << results_.seed << "\n"
            << results_.players.size() << "\n"
            << (results_.mode == game_mode::kBoss) << "\n"
            << (results_.mode == game_mode::kHard) << "\n"
            << (results_.mode == game_mode::kFast) << "\n"
            << (results_.mode == game_mode::kWhat) << "\n"
            << get_score() << "\n"
            << std::flush;
  throw score_finished{};
#endif

  if (!is_high_score()) {
    if (lib.is_key_pressed(Lib::key::kMenu)) {
      game_.sim_state().write_replay("untitled", get_score());
      save_.save();
      lib.play_sound(Lib::sound::kMenuAccept);
      quit();
    }
    return;
  }

  ++enter_time_;
  if (lib.is_key_pressed(Lib::key::kAccept) && enter_name_.length() < HighScores::kMaxNameLength) {
    enter_name_ += kAllowedChars.substr(enter_char_, 1);
    enter_time_ = 0;
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kCancel) && enter_name_.length() > 0) {
    enter_name_ = enter_name_.substr(0, enter_name_.length() - 1);
    enter_time_ = 0;
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kRight) || lib.is_key_pressed(Lib::key::kLeft)) {
    enter_r_ = 0;
  }
  if (lib.is_key_held(Lib::key::kRight) || lib.is_key_held(Lib::key::kLeft)) {
    ++enter_r_;
    enter_time_ = 16;
  }
  if (lib.is_key_pressed(Lib::key::kRight) ||
      (lib.is_key_held(Lib::key::kRight) && enter_r_ % 5 == 0 && enter_r_ > 5)) {
    enter_char_ = (enter_char_ + 1) % kAllowedChars.length();
    lib.play_sound(Lib::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kLeft) ||
      (lib.is_key_held(Lib::key::kLeft) && enter_r_ % 5 == 0 && enter_r_ > 5)) {
    enter_char_ = (enter_char_ + kAllowedChars.length() - 1) % kAllowedChars.length();
    lib.play_sound(Lib::sound::kMenuClick);
  }

  if (lib.is_key_pressed(Lib::key::kMenu)) {
    lib.play_sound(Lib::sound::kMenuAccept);
    save_.high_scores.add_score(results_.mode, results_.players.size() - 1, enter_name_,
                                get_score());
    game_.sim_state().write_replay(enter_name_, get_score());
    save_.save();
    quit();
  }
}

void HighScoreModal::render(Lib& lib) const {
  std::int32_t players = results_.players.size();
  if (is_high_score()) {
    render_panel(lib, {3.f, 20.f}, {28.f, 27.f});
    lib.render_text({4.f, 21.f}, "It's a high score!", z0Game::kPanelText);
    lib.render_text({4.f, 23.f},
                    players == 1 ? "Enter name:" : "Enter team name:", z0Game::kPanelText);
    lib.render_text({6.f, 25.f}, enter_name_, z0Game::kPanelText);
    if ((enter_time_ / 16) % 2 && enter_name_.length() < HighScores::kMaxNameLength) {
      lib.render_text({6.f + enter_name_.length(), 25.f}, kAllowedChars.substr(enter_char_, 1),
                      0xbbbbbbff);
    }
    fvec2 low{4 * Lib::kTextWidth + 4, 25 * Lib::kTextHeight + 4};
    fvec2 hi{5 * Lib::kTextWidth - 4, 26 * Lib::kTextHeight - 4};
    lib.render_rect(low, hi, z0Game::kPanelText, 1);
  }

  if (results_.mode == game_mode::kBoss) {
    std::int32_t extra_lives = results_.lives_remaining;
    bool b = extra_lives > 0 && results_.killed_bosses >= 6;

    long score = results_.elapsed_time;
    if (b) {
      score -= 10 * extra_lives;
    }
    if (score <= 0) {
      score = 1;
    }

    render_panel(lib, {3.f, 3.f}, {37.f, b ? 10.f : 8.f});
    if (b) {
      std::stringstream ss;
      ss << (extra_lives * 10) << "-second extra-life bonus!";
      lib.render_text({4.f, 4.f}, ss.str(), z0Game::kPanelText);
    }

    lib.render_text({4.f, b ? 6.f : 4.f}, "TIME ELAPSED: " + convert_to_time(score),
                    z0Game::kPanelText);
    std::stringstream ss;
    ss << "BOSS DESTROY: " << results_.killed_bosses;
    lib.render_text({4.f, b ? 8.f : 6.f}, ss.str(), z0Game::kPanelText);
    return;
  }

  render_panel(lib, {3.f, 3.f}, {37.f, 8.f + 2 * players + (players > 1 ? 2 : 0)});

  std::stringstream ss;
  ss << get_score();
  std::string score = ss.str();
  if (score.length() > HighScores::kMaxScoreLength) {
    score = score.substr(0, HighScores::kMaxScoreLength);
  }
  lib.render_text({4.f, 4.f}, "TOTAL SCORE: " + score, z0Game::kPanelText);

  for (std::int32_t i = 0; i < players; ++i) {
    std::stringstream ss;
    if (timer_ % 600 < 300) {
      ss << results_.players[i].score;
    } else {
      auto deaths = results_.players[i].deaths;
      ss << deaths << " death" << (deaths != 1 ? "s" : "");
    }
    score = ss.str();
    if (score.length() > HighScores::kMaxScoreLength) {
      score = score.substr(0, HighScores::kMaxScoreLength);
    }

    ss.str({});
    ss << "PLAYER " << (i + 1) << ":";
    lib.render_text({4.f, 8.f + 2 * i}, ss.str(), z0Game::kPanelText);
    lib.render_text({14.f, 8.f + 2 * i}, score, Player::player_colour(i));
  }

  if (players <= 1) {
    return;
  }

  bool first = true;
  std::int64_t max = 0;
  std::size_t best = 0;
  for (const auto& p : results_.players) {
    if (first || p.score > max) {
      max = p.score;
      best = p.number;
    }
    first = false;
  }

  if (get_score() > 0) {
    std::stringstream s;
    s << "PLAYER " << (best + 1);
    lib.render_text({4.f, 8.f + 2 * players}, s.str(), Player::player_colour(best));

    std::string compliment = kCompliments[compliment_];
    lib.render_text({12.f, 8.f + 2 * players}, compliment, z0Game::kPanelText);
  } else {
    lib.render_text({4.f, 8.f + 2 * players}, "Oh dear!", z0Game::kPanelText);
  }
}

std::int64_t HighScoreModal::get_score() const {
  if (results_.mode == game_mode::kBoss) {
    bool won = results_.killed_bosses >= 6 && results_.elapsed_time != 0;
    return !won ? 0l : std::max(1l, results_.elapsed_time - 600l * results_.lives_remaining);
  }
  std::int64_t total = 0;
  for (const auto& p : results_.players) {
    total += p.score;
  }
  return total;
}

bool HighScoreModal::is_high_score() const {
  return !results_.is_replay &&
      save_.high_scores.is_high_score(results_.mode, results_.players.size() - 1, get_score());
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
                     game_mode mode, std::int32_t player_count, bool can_face_secret_boss)
: Modal{true, true}, save_{save}, settings_{settings}, frame_count_{frame_count} {
  state_ =
      std::make_unique<SimState>(lib, save, frame_count, mode, player_count, can_face_secret_boss);
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings, std::int32_t* frame_count,
                     const std::string& replay_path)
: Modal{true, true}, save_{save}, settings_{settings}, frame_count_{frame_count} {
  state_ = std::make_unique<SimState>(lib, save, frame_count, replay_path);
}

GameModal::~GameModal() {}

void GameModal::update(Lib& lib) {
  if (pause_output_ == PauseModal::kEndGame || state_->game_over()) {
    add(std::make_unique<HighScoreModal>(save_, *this, state_->get_results()));
    *frame_count_ = 1;
    if (pause_output_ != PauseModal::kEndGame) {
      lib.play_sound(Lib::sound::kMenuAccept);
    }
    quit();
    return;
  }

  if (lib.is_key_pressed(Lib::key::kMenu)) {
    lib.capture_mouse(false);
    add(std::make_unique<PauseModal>(&pause_output_, settings_));
    lib.play_sound(Lib::sound::kMenuAccept);
    return;
  }
  lib.capture_mouse(true);

  auto results = state_->get_results();
  std::int32_t controllers = 0;
  for (std::size_t i = 0; i < results.players.size(); ++i) {
    controllers |= lib.get_pad_type(i);
  }
  if (controllers < controllers_connected_ && !controllers_dialog_ && !results.is_replay) {
    controllers_dialog_ = true;
    lib.play_sound(Lib::sound::kMenuAccept);
  }
  controllers_connected_ = controllers;

  if (controllers_dialog_) {
    if (lib.is_key_pressed(Lib::key::kMenu) || lib.is_key_pressed(Lib::key::kAccept)) {
      controllers_dialog_ = false;
      lib.play_sound(Lib::sound::kMenuAccept);
      for (std::size_t i = 0; i < results.players.size(); ++i) {
        lib.rumble(i, 10);
      }
    }
    return;
  }

  state_->update(lib);
}

void GameModal::render(Lib& lib) const {
  state_->render(lib);
  auto results = state_->get_results();

  if (controllers_dialog_) {
    render_panel(lib, {3.f, 3.f}, {32.f, 8.f + 2 * results.players.size()});
    lib.render_text({4.f, 4.f}, "CONTROLLERS FOUND", z0Game::kPanelText);

    for (std::size_t i = 0; i < results.players.size(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      lib.render_text({4.f, 8.f + 2 * i}, ss.str(), z0Game::kPanelText);

      ss.str({});
      std::int32_t pads = lib.get_pad_type(i);
      if (results.is_replay) {
        ss << "REPLAY";
        pads = 1;
      } else {
        if (!pads) {
          ss << "NONE";
        }
        if (pads & Lib::pad_type::kPadGamepad) {
          ss << "GAMEPAD";
          if (pads & Lib::pad_type::kPadKeyboardMouse) {
            ss << ", KB/MOUSE";
          }
        } else if (pads & Lib::pad_type::kPadKeyboardMouse) {
          ss << "MOUSE & KEYBOARD";
        }
      }

      lib.render_text({14.f, 8.f + 2 * i}, ss.str(),
                      pads ? Player::player_colour(i) : z0Game::kPanelText);
    }
    return;
  }

  std::stringstream ss;
  ss << results.lives_remaining << " live(s)";
  if (results.mode != game_mode::kBoss && results.overmind_timer >= 0) {
    auto t = static_cast<std::int32_t>(0.5f + results.overmind_timer / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  lib.render_text({Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                   Lib::kHeight / Lib::kTextHeight - 2.f},
                  ss.str(), z0Game::kPanelTran);

  if (results.mode == game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(results.elapsed_time);
    lib.render_text({Lib::kWidth / (2 * Lib::kTextWidth) - ss.str().length() - 1.f, 1.f}, ss.str(),
                    z0Game::kPanelTran);
  }

  if (results.boss_hp_bar) {
    std::int32_t x = results.mode == game_mode::kBoss ? 48 : 0;
    lib.render_rect({x + Lib::kWidth / 2 - 48.f, 16.f}, {x + Lib::kWidth / 2 + 48.f, 32.f},
                    z0Game::kPanelTran, 2);

    lib.render_rect({x + Lib::kWidth / 2 - 44.f, 16.f + 4},
                    {x + Lib::kWidth / 2 - 44.f + 88.f * *results.boss_hp_bar, 32.f - 4},
                    z0Game::kPanelTran);
  }

  if (results.is_replay) {
    std::int32_t x = results.mode == game_mode::kFast ? *frame_count_ / 2 : *frame_count_;
    ss.str({});
    ss << x << "X " << static_cast<std::int32_t>(100 * results.replay_progress) << "%";

    lib.render_text({Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                     Lib::kHeight / Lib::kTextHeight - 3.f},
                    ss.str(), z0Game::kPanelTran);
  }
}

z0Game::z0Game(Lib& lib, const std::vector<std::string>& args) : lib_{lib} {
  lib.set_volume(settings_.volume.to_int());

  if (!args.empty()) {
    try {
      modals_.add(std::make_unique<GameModal>(lib_, save_, settings_, &frame_count_, args[0]));
    } catch (const std::runtime_error& e) {
      exit_timer_ = 256;
      exit_error_ = e.what();
    }
  }
}

void z0Game::run() {
  while (true) {
    std::size_t f = frame_count_;
    if (!f) {
      continue;
    }

    for (std::size_t i = 0; i < f; ++i) {
#ifdef PLATFORM_SCORE
      frame_count_ = 16384;
#endif
      if (lib_.begin_frame() || update()) {
        lib_.end_frame();
        return;
      }
      lib_.end_frame();
    }

    lib_.clear_screen();
    render();
    lib_.render();
  }
}

bool z0Game::update() {
  if (exit_timer_) {
    exit_timer_--;
    return !exit_timer_;
  }

  for (std::int32_t i = 0; i < kPlayers; ++i) {
    if (lib().is_key_held(i, Lib::key::kFire) && lib().is_key_held(i, Lib::key::kBomb) &&
        lib().is_key_pressed(Lib::key::kMenu)) {
      lib().take_screenshot();
      break;
    }
  }
  lib().capture_mouse(false);

  if (modals_.update(lib_)) {
    return false;
  }

  menu t = menu_select_;
  if (lib().is_key_pressed(Lib::key::kUp)) {
    menu_select_ = static_cast<menu>(
        std::max(static_cast<std::int32_t>(menu_select_) - 1,
                 static_cast<std::int32_t>(mode_unlocked() >= game_mode::kBoss ? menu::kSpecial
                                                                               : menu::kStart)));
  }
  if (lib().is_key_pressed(Lib::key::kDown)) {
    menu_select_ = static_cast<menu>(std::min(static_cast<std::int32_t>(menu::kQuit),
                                              static_cast<std::int32_t>(menu_select_) + 1));
  }
  if (t != menu_select_) {
    lib().play_sound(Lib::sound::kMenuClick);
  }

  if (menu_select_ == menu::kPlayers) {
    std::int32_t t = player_select_;
    if (lib().is_key_pressed(Lib::key::kLeft)) {
      player_select_ = std::max(1, player_select_ - 1);
    }
    if (lib().is_key_pressed(Lib::key::kRight)) {
      player_select_ = std::min(kPlayers, player_select_ + 1);
    }
    if (lib().is_key_pressed(Lib::key::kAccept) || lib().is_key_pressed(Lib::key::kMenu)) {
      player_select_ = 1 + player_select_ % kPlayers;
    }
    if (t != player_select_) {
      lib().play_sound(Lib::sound::kMenuClick);
    }
    lib().set_player_count(player_select_);
  }

  if (menu_select_ == menu::kSpecial) {
    game_mode t = mode_select_;
    if (lib().is_key_pressed(Lib::key::kLeft)) {
      mode_select_ = static_cast<game_mode>(std::max(static_cast<std::int32_t>(game_mode::kBoss),
                                                     static_cast<std::int32_t>(mode_select_) - 1));
    }
    if (lib().is_key_pressed(Lib::key::kRight)) {
      mode_select_ = static_cast<game_mode>(std::min(static_cast<std::int32_t>(mode_unlocked()),
                                                     static_cast<std::int32_t>(mode_select_) + 1));
    }
    if (t != mode_select_) {
      lib().play_sound(Lib::sound::kMenuClick);
    }
  }

  if (lib().is_key_pressed(Lib::key::kAccept) || lib().is_key_pressed(Lib::key::kMenu)) {
    if (menu_select_ == menu::kStart) {
      lib().new_game();
      modals_.add(std::make_unique<GameModal>(lib_, save_, settings_, &frame_count_,
                                              game_mode::kNormal, player_select_,
                                              mode_unlocked() >= game_mode::kFast));
    } else if (menu_select_ == menu::kQuit) {
      exit_timer_ = 2;
    } else if (menu_select_ == menu::kSpecial) {
      lib().new_game();
      modals_.add(std::make_unique<GameModal>(lib_, save_, settings_, &frame_count_, mode_select_,
                                              player_select_, mode_unlocked() >= game_mode::kFast));
    }
    if (menu_select_ != menu::kPlayers) {
      lib().play_sound(Lib::sound::kMenuAccept);
    }
  }

  if (menu_select_ >= menu::kStart || mode_select_ == game_mode::kBoss) {
    lib().set_colour_cycle(0);
  } else if (mode_select_ == game_mode::kHard) {
    lib().set_colour_cycle(128);
  } else if (mode_select_ == game_mode::kFast) {
    lib().set_colour_cycle(192);
  } else if (mode_select_ == game_mode::kWhat) {
    lib().set_colour_cycle((lib().get_colour_cycle() + 1) % 256);
  }
  return false;
}

void z0Game::render() const {
  if (exit_timer_ > 0 && !exit_error_.empty()) {
    std::int32_t y = 2;
    std::string s = exit_error_;
    std::size_t i;
    while ((i = s.find_first_of("\n")) != std::string::npos) {
      std::string t = s.substr(0, i);
      s = s.substr(i + 1);
      lib().render_text({2, static_cast<float>(y)}, t, kPanelText);
      ++y;
    }
    lib().render_text({2, static_cast<float>(y)}, s, kPanelText);
    return;
  }

  if (modals_.render(lib_)) {
    return;
  }

  render_panel(lib(), {3.f, 3.f}, {19.f, 14.f});

  lib().render_text({37.f - 16, 3.f}, "coded by: SEiKEN", kPanelText);
  lib().render_text({37.f - 16, 4.f}, "stu@seiken.co.uk", kPanelText);
  lib().render_text({37.f - 9, 6.f}, "-testers-", kPanelText);
  lib().render_text({37.f - 9, 7.f}, "MATT BELL", kPanelText);
  lib().render_text({37.f - 9, 8.f}, "RUFUZZZZZ", kPanelText);
  lib().render_text({37.f - 9, 9.f}, "SHADOW1W2", kPanelText);

  std::string b = "BOSSES:  ";
  std::int32_t bb =
      mode_unlocked() >= game_mode::kHard ? save_.hard_mode_bosses_killed : save_.bosses_killed;
  b += bb & SimState::BOSS_1A ? "X" : "-";
  b += bb & SimState::BOSS_1B ? "X" : "-";
  b += bb & SimState::BOSS_1C ? "X" : "-";
  b += bb & SimState::BOSS_3A ? "X" : " ";
  b += bb & SimState::BOSS_2A ? "X" : "-";
  b += bb & SimState::BOSS_2B ? "X" : "-";
  b += bb & SimState::BOSS_2C ? "X" : "-";
  lib().render_text({37.f - 16, 13.f}, b, kPanelText);

  lib().render_text({4.f, 4.f}, "WiiSPACE", kPanelText);
  lib().render_text({6.f, 8.f}, "START GAME", kPanelText);
  lib().render_text({6.f, 10.f}, "PLAYERS", kPanelText);
  lib().render_text({6.f, 12.f}, "EXiT", kPanelText);

  if (mode_unlocked() >= game_mode::kBoss) {
    std::string str = mode_select_ == game_mode::kBoss ? "BOSS MODE"
        : mode_select_ == game_mode::kHard             ? "HARD MODE"
        : mode_select_ == game_mode::kFast             ? "FAST MODE"
                                                       : "W-HAT MODE";
    lib().render_text({6.f, 6.f}, str, kPanelText);
  }
  if (menu_select_ == menu::kSpecial && mode_select_ > game_mode::kBoss) {
    lib().render_text({5.f, 6.f}, "<", kPanelTran);
  }
  if (menu_select_ == menu::kSpecial && mode_select_ < mode_unlocked()) {
    lib().render_text({6.f, 6.f}, "         >", kPanelTran);
  }

  fvec2 low{
      static_cast<float>(4 * Lib::kTextWidth + 4),
      static_cast<float>((6 + 2 * static_cast<std::int32_t>(menu_select_)) * Lib::kTextHeight + 4)};
  fvec2 hi{
      static_cast<float>(5 * Lib::kTextWidth - 4),
      static_cast<float>((7 + 2 * static_cast<std::int32_t>(menu_select_)) * Lib::kTextHeight - 4)};
  lib().render_rect(low, hi, kPanelText, 1);

  if (player_select_ > 1 && menu_select_ == menu::kPlayers) {
    lib().render_text({5.f, 10.f}, "<", kPanelTran);
  }
  if (player_select_ < 4 && menu_select_ == menu::kPlayers) {
    lib().render_text({14.f + player_select_, 10.f}, ">", kPanelTran);
  }
  for (std::int32_t i = 0; i < player_select_; ++i) {
    std::stringstream ss;
    ss << (i + 1);
    lib().render_text({14.f + i, 10.f}, ss.str(), Player::player_colour(i));
  }

  render_panel(lib(), {3.f, 15.f}, {37.f, 27.f});

  std::stringstream ss;
  ss << player_select_;
  std::string s = "HiGH SCORES    ";
  s += menu_select_ == menu::kSpecial
      ? (mode_select_ == game_mode::kBoss       ? "BOSS MODE"
             : mode_select_ == game_mode::kHard ? "HARD MODE (" + ss.str() + "P)"
             : mode_select_ == game_mode::kFast ? "FAST MODE (" + ss.str() + "P)"
                                                : "W-HAT MODE (" + ss.str() + "P)")
      : player_select_ == 1 ? "ONE PLAYER"
      : player_select_ == 2 ? "TWO PLAYERS"
      : player_select_ == 3 ? "THREE PLAYERS"
      : player_select_ == 4 ? "FOUR PLAYERS"
                            : "";
  lib().render_text({4.f, 16.f}, s, kPanelText);

  if (menu_select_ == menu::kSpecial && mode_select_ == game_mode::kBoss) {
    lib().render_text({4.f, 18.f}, "ONE PLAYER", kPanelText);
    lib().render_text({4.f, 20.f}, "TWO PLAYERS", kPanelText);
    lib().render_text({4.f, 22.f}, "THREE PLAYERS", kPanelText);
    lib().render_text({4.f, 24.f}, "FOUR PLAYERS", kPanelText);

    for (std::size_t i = 0; i < kPlayers; ++i) {
      auto& s = save_.high_scores.get(game_mode::kBoss, i, 0);
      std::string score = convert_to_time(s.score).substr(0, HighScores::kMaxNameLength);
      std::string name = s.name.substr(HighScores::kMaxNameLength);

      lib().render_text({19.f, 18.f + i * 2}, score, kPanelText);
      lib().render_text({19.f, 19.f + i * 2}, name, kPanelText);
    }
  } else {
    for (std::size_t i = 0; i < HighScores::kNumScores; ++i) {
      ss.str({});
      ss << (i + 1) << ".";
      lib().render_text({4.f, 18.f + i}, ss.str(), kPanelText);

      auto& s =
          save_.high_scores.get(menu_select_ == menu::kSpecial ? mode_select_ : game_mode::kNormal,
                                player_select_ - 1, i);
      if (s.score <= 0) {
        continue;
      }

      ss.str({});
      ss << s.score;
      std::string score = ss.str().substr(0, HighScores::kMaxScoreLength);
      std::string name = s.name.substr(0, HighScores::kMaxScoreLength);

      lib().render_text({7.f, 18.f + i}, score, kPanelText);
      lib().render_text({19.f, 18.f + i}, name, kPanelText);
    }
  }
}

game_mode z0Game::mode_unlocked() const {
  if (!(save_.bosses_killed & 63)) {
    return game_mode::kNormal;
  }
  if (save_.high_scores.boss[0].score == 0 && save_.high_scores.boss[1].score == 0 &&
      save_.high_scores.boss[2].score == 0 && save_.high_scores.boss[3].score == 0) {
    return game_mode::kBoss;
  }
  if ((save_.hard_mode_bosses_killed & 63) != 63) {
    return game_mode::kHard;
  }
  if ((save_.hard_mode_bosses_killed & 64) != 64) {
    return game_mode::kFast;
  }
  return game_mode::kWhat;
}
