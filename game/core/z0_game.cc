#include "game/core/z0_game.h"
#include "game/core/lib.h"
#include "game/io/file/filesystem.h"
#include "game/logic/sim/sim_interface.h"
#include <algorithm>
#include <iostream>
#include <sstream>

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

void save_replay(ii::io::Filesystem& fs, const ii::ReplayWriter& writer, const std::string& name,
                 std::int64_t score) {
  std::stringstream ss;
  auto mode = writer.initial_conditions().mode;
  ss << writer.initial_conditions().seed << "_" << writer.initial_conditions().player_count << "p_"
     << (mode == game_mode::kBoss       ? "bossmode_"
             : mode == game_mode::kHard ? "hardmode_"
             : mode == game_mode::kFast ? "fastmode_"
             : mode == game_mode::kWhat ? "w-hatmode_"
                                        : "")
     << name << "_" << score;

  auto data = writer.write();
  if (data) {
    (void)fs.write_replay(ss.str(), *data);
  }
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
    lib.play_sound(ii::sound::kMenuClick);
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
    lib.play_sound(ii::sound::kMenuAccept);
  }

  if (lib.is_key_pressed(Lib::key::kCancel)) {
    quit();
    lib.play_sound(ii::sound::kMenuAccept);
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
    lib.play_sound(ii::sound::kMenuClick);
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

HighScoreModal::HighScoreModal(bool is_replay, SaveData& save, GameModal& game,
                               const ii::SimState::results& results,
                               ii::ReplayWriter* replay_writer)
: Modal{true, false}
, is_replay_{is_replay}
, save_{save}
, game_{game}
, results_{results}
, replay_writer_{replay_writer}
, compliment_{z::rand_int(kCompliments.size())} {
  if (!is_replay) {
    save_.bosses_killed |= results.bosses_killed;
    save_.hard_mode_bosses_killed |= results.hard_mode_bosses_killed;
  }
}

void HighScoreModal::update(Lib& lib) {
  ++timer_;
  if (!is_high_score()) {
    if (lib.is_key_pressed(Lib::key::kMenu)) {
      if (replay_writer_) {
        save_replay(lib.filesystem(), *replay_writer_, "untitled", get_score());
      }
      save_.save();
      lib.play_sound(ii::sound::kMenuAccept);
      quit();
    }
    return;
  }

  ++enter_time_;
  if (lib.is_key_pressed(Lib::key::kAccept) && enter_name_.length() < HighScores::kMaxNameLength) {
    enter_name_ += kAllowedChars.substr(enter_char_, 1);
    enter_time_ = 0;
    lib.play_sound(ii::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kCancel) && enter_name_.length() > 0) {
    enter_name_ = enter_name_.substr(0, enter_name_.length() - 1);
    enter_time_ = 0;
    lib.play_sound(ii::sound::kMenuClick);
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
    lib.play_sound(ii::sound::kMenuClick);
  }
  if (lib.is_key_pressed(Lib::key::kLeft) ||
      (lib.is_key_held(Lib::key::kLeft) && enter_r_ % 5 == 0 && enter_r_ > 5)) {
    enter_char_ = (enter_char_ + kAllowedChars.length() - 1) % kAllowedChars.length();
    lib.play_sound(ii::sound::kMenuClick);
  }

  if (lib.is_key_pressed(Lib::key::kMenu)) {
    lib.play_sound(ii::sound::kMenuAccept);
    save_.high_scores.add_score(results_.mode, results_.players.size() - 1, enter_name_,
                                get_score());
    if (replay_writer_) {
      save_replay(lib.filesystem(), *replay_writer_, enter_name_, get_score());
    }
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
    lib.render_text({14.f, 8.f + 2 * i}, score, ii::SimInterface::player_colour(i));
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
    lib.render_text({4.f, 8.f + 2 * players}, s.str(), ii::SimInterface::player_colour(best));

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
  return !is_replay_ &&
      save_.high_scores.is_high_score(results_.mode, results_.players.size() - 1, get_score());
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings,
                     const frame_count_callback& callback, game_mode mode,
                     std::int32_t player_count, bool can_face_secret_boss)
: Modal{true, true}, save_{save}, settings_{settings}, callback_{callback} {
  std::int32_t seed = static_cast<std::int32_t>(time(0));
  ii::initial_conditions conditions{seed, mode, player_count, can_face_secret_boss};
  lib.set_player_count(player_count);
  lib.new_game();
  game_.emplace(lib, ii::ReplayWriter{conditions});
  state_ = std::make_unique<ii::SimState>(conditions, game_->input);
}

GameModal::GameModal(Lib& lib, SaveData& save, Settings& settings,
                     const frame_count_callback& callback, const std::string& replay_path)
: Modal{true, true}, save_{save}, settings_{settings}, callback_{callback} {
  // TODO: exceptions.
  auto replay_data = lib.filesystem().read_replay(replay_path);
  if (!replay_data) {
    throw std::runtime_error{replay_data.error()};
  }
  auto reader = ii::ReplayReader::create(*replay_data);
  if (!reader) {
    throw std::runtime_error{reader.error()};
  }

  auto conditions = reader->initial_conditions();
  lib.set_player_count(conditions.player_count);
  lib.new_game();
  replay_.emplace(std::move(*reader));
  state_ = std::make_unique<ii::SimState>(conditions, replay_->input);
}

GameModal::~GameModal() {}

void GameModal::update(Lib& lib) {
  if (pause_output_ == PauseModal::kEndGame || state_->game_over()) {
    callback_(1);
    add(std::make_unique<HighScoreModal>(replay_.has_value(), save_, *this, state_->get_results(),
                                         game_ ? &game_->writer : nullptr));
    if (pause_output_ != PauseModal::kEndGame) {
      lib.play_sound(ii::sound::kMenuAccept);
    }
    quit();
    return;
  }

  if (lib.is_key_pressed(Lib::key::kMenu) && !controllers_dialog_) {
    lib.capture_mouse(false);
    add(std::make_unique<PauseModal>(&pause_output_, settings_));
    lib.play_sound(ii::sound::kMenuAccept);
    return;
  }
  lib.capture_mouse(true);

  std::int32_t controllers = 0;
  for (std::size_t i = 0; i < lib.player_count(); ++i) {
    controllers |= lib.get_pad_type(i);
  }
  if (controllers < controllers_connected_ && !controllers_dialog_ && !replay_) {
    controllers_dialog_ = true;
    lib.play_sound(ii::sound::kMenuAccept);
  }
  controllers_connected_ = controllers;

  if (controllers_dialog_) {
    if (lib.is_key_pressed(Lib::key::kMenu) || lib.is_key_pressed(Lib::key::kAccept)) {
      controllers_dialog_ = false;
      lib.play_sound(ii::sound::kMenuAccept);
      for (std::size_t i = 0; i < lib.player_count(); ++i) {
        lib.rumble(i, 10);
      }
    }
    return;
  }

  state_->update();
  lib.post_update(*state_);
  if (replay_) {
    if (lib.is_key_pressed(Lib::key::kBomb)) {
      frame_count_multiplier_ *= 2;
    }
    if (lib.is_key_pressed(Lib::key::kFire) && frame_count_multiplier_ > 1) {
      frame_count_multiplier_ /= 2;
    }
  }
  callback_(state_->frame_count() * frame_count_multiplier_);
}

void GameModal::render(Lib& lib) const {
  state_->render();
  auto render = state_->get_render_output();
  lib.set_colour_cycle(render.colour_cycle);

  for (const auto& line : render.lines) {
    lib.render_line(line.a, line.b, line.c);
  }

  std::int32_t n = 0;
  for (const auto& p : render.players) {
    std::stringstream ss;
    ss << p.multiplier << "X";
    std::string s = ss.str();
    fvec2 v = n == 1 ? fvec2{Lib::kWidth / 16 - 1.f - s.length(), 1.f}
        : n == 2     ? fvec2{1.f, Lib::kHeight / 16 - 2.f}
        : n == 3     ? fvec2{Lib::kWidth / 16 - 1.f - s.length(), Lib::kHeight / 16 - 2.f}
                     : fvec2{1.f, 1.f};
    lib.render_text(v, s, z0Game::kPanelText);

    ss.str("");
    n % 2 ? ss << p.score << "   " : ss << "   " << p.score;
    lib.render_text(
        v - (n % 2 ? fvec2{static_cast<float>(ss.str().length() - s.length()), 0} : fvec2{}),
        ss.str(), p.colour);

    if (p.timer) {
      v.x += n % 2 ? -1 : ss.str().length();
      v *= 16;
      auto lo = v + fvec2{5.f, 11.f - 10 * p.timer};
      auto hi = v + fvec2{9.f, 13.f};
      lib.render_line(lo, {lo.x, hi.y}, 0xffffffff);
      lib.render_line({lo.x, hi.y}, hi, 0xffffffff);
      lib.render_line(hi, {hi.x, lo.y}, 0xffffffff);
      lib.render_line({hi.x, lo.y}, lo, 0xffffffff);
    }
    ++n;
  }

  if (controllers_dialog_) {
    render_panel(lib, {3.f, 3.f}, {32.f, 8.f + 2 * lib.player_count()});
    lib.render_text({4.f, 4.f}, "CONTROLLERS FOUND", z0Game::kPanelText);

    for (std::size_t i = 0; i < lib.player_count(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      lib.render_text({4.f, 8.f + 2 * i}, ss.str(), z0Game::kPanelText);

      ss.str({});
      std::int32_t pads = lib.get_pad_type(i);
      if (replay_) {
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
                      pads ? ii::SimInterface::player_colour(i) : z0Game::kPanelText);
    }
    return;
  }

  std::stringstream ss;
  ss << render.lives_remaining << " live(s)";
  if (render.mode != game_mode::kBoss && render.overmind_timer >= 0) {
    auto t = static_cast<std::int32_t>(0.5f + render.overmind_timer / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  lib.render_text({Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                   Lib::kHeight / Lib::kTextHeight - 2.f},
                  ss.str(), z0Game::kPanelTran);

  if (render.mode == game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(render.elapsed_time);
    lib.render_text({Lib::kWidth / (2 * Lib::kTextWidth) - ss.str().length() - 1.f, 1.f}, ss.str(),
                    z0Game::kPanelTran);
  }

  if (render.boss_hp_bar) {
    std::int32_t x = render.mode == game_mode::kBoss ? 48 : 0;
    lib.render_rect({x + Lib::kWidth / 2 - 48.f, 16.f}, {x + Lib::kWidth / 2 + 48.f, 32.f},
                    z0Game::kPanelTran, 2);

    lib.render_rect({x + Lib::kWidth / 2 - 44.f, 16.f + 4},
                    {x + Lib::kWidth / 2 - 44.f + 88.f * *render.boss_hp_bar, 32.f - 4},
                    z0Game::kPanelTran);
  }

  if (replay_) {
    auto progress = static_cast<float>(replay_->reader.current_input_frame()) /
        replay_->reader.total_input_frames();
    std::int32_t x =
        render.mode == game_mode::kFast ? state_->frame_count() / 2 : state_->frame_count();
    ss.str({});
    ss << x << "X " << static_cast<std::int32_t>(100 * progress) << "%";

    lib.render_text({Lib::kWidth / (2.f * Lib::kTextWidth) - ss.str().length() / 2,
                     Lib::kHeight / Lib::kTextHeight - 3.f},
                    ss.str(), z0Game::kPanelTran);
  }
}

z0Game::z0Game(Lib& lib, const std::vector<std::string>& args)
: lib_{lib}, save_{lib.filesystem()}, settings_{lib.filesystem()} {
  lib.set_volume(settings_.volume.to_int());

  if (!args.empty()) {
    try {
      modals_.add(std::make_unique<GameModal>(
          lib_, save_, settings_, [this](std::int32_t c) { frame_count_ = c; }, args[0]));
    } catch (const std::runtime_error& e) {
      exit_timer_ = 256;
      exit_error_ = e.what();
    }
  }
}

void z0Game::run() {
  while (true) {
    std::size_t f = frame_count_;
    for (std::size_t i = 0; i < f; ++i) {
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
    lib().play_sound(ii::sound::kMenuClick);
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
      lib().play_sound(ii::sound::kMenuClick);
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
      lib().play_sound(ii::sound::kMenuClick);
    }
  }

  if (lib().is_key_pressed(Lib::key::kAccept) || lib().is_key_pressed(Lib::key::kMenu)) {
    if (menu_select_ == menu::kStart) {
      lib().new_game();
      modals_.add(std::make_unique<GameModal>(
          lib_, save_, settings_, [this](std::int32_t c) { frame_count_ = c; }, game_mode::kNormal,
          player_select_, mode_unlocked() >= game_mode::kFast));
    } else if (menu_select_ == menu::kQuit) {
      exit_timer_ = 2;
    } else if (menu_select_ == menu::kSpecial) {
      lib().new_game();
      modals_.add(std::make_unique<GameModal>(
          lib_, save_, settings_, [this](std::int32_t c) { frame_count_ = c; }, mode_select_,
          player_select_, mode_unlocked() >= game_mode::kFast));
    }
    if (menu_select_ != menu::kPlayers) {
      lib().play_sound(ii::sound::kMenuAccept);
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
  b += bb & ii::SimInterface::kBoss1A ? "X" : "-";
  b += bb & ii::SimInterface::kBoss1B ? "X" : "-";
  b += bb & ii::SimInterface::kBoss1C ? "X" : "-";
  b += bb & ii::SimInterface::kBoss3A ? "X" : " ";
  b += bb & ii::SimInterface::kBoss2A ? "X" : "-";
  b += bb & ii::SimInterface::kBoss2B ? "X" : "-";
  b += bb & ii::SimInterface::kBoss2C ? "X" : "-";
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
    lib().render_text({14.f + i, 10.f}, ss.str(), ii::SimInterface::player_colour(i));
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
