#include "game/core/z0_game.h"
#include "game/core/ui_layer.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/logic/sim/sim_interface.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <sstream>

namespace {
constexpr std::int32_t kWidth = 640;
constexpr std::int32_t kHeight = 480;
constexpr std::int32_t kTextWidth = 16;
constexpr std::int32_t kTextHeight = 16;

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

glm::vec4 convert_colour(colour_t c) {
  return {((c >> 24) & 0xff) / 255.f, ((c >> 16) & 0xff) / 255.f, ((c >> 8) & 0xff) / 255.f,
          (c & 0xff) / 255.f};
}

glm::vec2 convert_vec(fvec2 v) {
  return {v.x, v.y};
}

void render_line(ii::render::GlRenderer& r, const fvec2& a, const fvec2& b, colour_t c) {
  c = z::colour_cycle(c, r.colour_cycle());
  ii::render::line_t line{convert_vec(a), convert_vec(b), convert_colour(c)};
  r.render_lines({&line, 1});
}

void render_lines(ii::render::GlRenderer& r, const nonstd::span<ii::render_output::line_t>& lines) {
  std::vector<ii::render::line_t> render;
  for (const auto& line : lines) {
    auto& rl = render.emplace_back();
    rl.a = convert_vec(line.a);
    rl.b = convert_vec(line.b);
    rl.colour = convert_colour(z::colour_cycle(line.c, r.colour_cycle()));
  }
  r.render_lines(render);
}

void render_text(ii::render::GlRenderer& r, const fvec2& v, const std::string& text, colour_t c) {
  c = z::colour_cycle(c, r.colour_cycle());
  r.render_text(0, 16 * static_cast<glm::ivec2>(convert_vec(v)), convert_colour(c),
                ii::ustring_view::utf8(text));
}

void render_rect(ii::render::GlRenderer& r, const fvec2& lo, const fvec2& hi, colour_t c,
                 std::int32_t line_width) {
  c = z::colour_cycle(c, r.colour_cycle());
  auto v_lo = static_cast<glm::ivec2>(convert_vec(lo));
  auto v_hi = static_cast<glm::ivec2>(convert_vec(hi));
  r.render_rect(v_lo, v_hi - v_lo, line_width, glm::vec4{0.f}, glm::vec4{0.f}, convert_colour(c),
                convert_colour(c));
}

void render_panel(ii::render::GlRenderer& r, const fvec2& low, const fvec2& hi) {
  fvec2 tlow{low.x * kTextWidth, low.y * kTextHeight};
  fvec2 thi{hi.x * kTextWidth, hi.y * kTextHeight};
  render_rect(r, tlow, thi, z0Game::kPanelBack, 2);
  render_rect(r, tlow, thi, z0Game::kPanelText, 4);
}

}  // namespace

PauseModal::PauseModal(output_t* output) : Modal{true, false}, output_{output} {
  *output = kContinue;
}

void PauseModal::update(ii::ui::UiLayer& ui) {
  std::int32_t t = selection_;
  if (ui.input().pressed(ii::ui::key::kUp)) {
    selection_ = std::max(0, selection_ - 1);
  }
  if (ui.input().pressed(ii::ui::key::kDown)) {
    selection_ = std::min(2, selection_ + 1);
  }
  if (t != selection_) {
    ui.play_sound(ii::sound::kMenuClick);
  }

  bool accept = ui.input().pressed(ii::ui::key::kAccept) || ui.input().pressed(ii::ui::key::kMenu);
  if (accept && selection_ == 0) {
    quit();
  } else if (accept && selection_ == 1) {
    *output_ = kEndGame;
    quit();
  } else if (accept && selection_ == 2) {
    ui.write_config();
    ui.set_volume(ui.config().volume);
  }
  if (accept) {
    ui.play_sound(ii::sound::kMenuAccept);
  }

  if (ui.input().pressed(ii::ui::key::kCancel)) {
    quit();
    ui.play_sound(ii::sound::kMenuAccept);
  }

  auto v = ui.config().volume;
  if (selection_ == 2 && ui.input().pressed(ii::ui::key::kLeft)) {
    ui.config().volume = std::max(0.f, ui.config().volume - 1.f);
  }
  if (selection_ == 2 && ui.input().pressed(ii::ui::key::kRight)) {
    ui.config().volume = std::min(100.f, ui.config().volume + 1.f);
  }
  if (v != ui.config().volume) {
    ui.set_volume(ui.config().volume);
    ui.write_config();
    ui.play_sound(ii::sound::kMenuClick);
  }
}

void PauseModal::render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const {
  render_panel(r, {3.f, 3.f}, {15.f, 14.f});

  render_text(r, {4.f, 4.f}, "PAUSED", z0Game::kPanelText);
  render_text(r, {6.f, 8.f}, "CONTINUE", z0Game::kPanelText);
  render_text(r, {6.f, 10.f}, "END GAME", z0Game::kPanelText);
  render_text(r, {6.f, 12.f}, "VOL.", z0Game::kPanelText);
  std::stringstream vol;
  auto v = static_cast<std::int32_t>(ui.config().volume);
  vol << " " << (v < 10 ? " " : "") << v;
  render_text(r, {10.f, 12.f}, vol.str(), z0Game::kPanelText);
  if (selection_ == 2 && v > 0) {
    render_text(r, {5.f, 12.f}, "<", z0Game::kPanelTran);
  }
  if (selection_ == 2 && v < 100) {
    render_text(r, {13.f, 12.f}, ">", z0Game::kPanelTran);
  }

  fvec2 low{static_cast<float>(4 * kTextWidth + 4),
            static_cast<float>((8 + 2 * selection_) * kTextHeight + 4)};
  fvec2 hi{static_cast<float>(5 * kTextWidth - 4),
           static_cast<float>((9 + 2 * selection_) * kTextHeight - 4)};
  render_rect(r, low, hi, z0Game::kPanelText, 1);
}

HighScoreModal::HighScoreModal(bool is_replay, GameModal& game, const ii::sim_results& results,
                               ii::ReplayWriter* replay_writer)
: Modal{true, false}
, is_replay_{is_replay}
, game_{game}
, results_{results}
, replay_writer_{replay_writer}
, compliment_{results.seed % static_cast<std::int32_t>(kCompliments.size())} {}

void HighScoreModal::update(ii::ui::UiLayer& ui) {
  if (!is_replay_) {
    ui.save_game().bosses_killed |= results_.bosses_killed;
    ui.save_game().hard_mode_bosses_killed |= results_.hard_mode_bosses_killed;
  }

  ++timer_;
  if (!is_high_score(ui.save_game())) {
    if (ui.input().pressed(ii::ui::key::kMenu)) {
      if (replay_writer_) {
        ui.write_replay(*replay_writer_, "untitled", get_score());
      }
      ui.write_save_game();
      ui.play_sound(ii::sound::kMenuAccept);
      quit();
    }
    return;
  }

  ++enter_time_;
  if (ui.input().pressed(ii::ui::key::kAccept) &&
      enter_name_.length() < ii::HighScores::kMaxNameLength) {
    enter_name_ += kAllowedChars.substr(enter_char_, 1);
    enter_time_ = 0;
    ui.play_sound(ii::sound::kMenuClick);
  }
  if (ui.input().pressed(ii::ui::key::kCancel) && enter_name_.length() > 0) {
    enter_name_ = enter_name_.substr(0, enter_name_.length() - 1);
    enter_time_ = 0;
    ui.play_sound(ii::sound::kMenuClick);
  }
  if (ui.input().pressed(ii::ui::key::kRight) || ui.input().pressed(ii::ui::key::kLeft)) {
    enter_r_ = 0;
  }
  if (ui.input().held(ii::ui::key::kRight) || ui.input().held(ii::ui::key::kLeft)) {
    ++enter_r_;
    enter_time_ = 16;
  }
  if (ui.input().pressed(ii::ui::key::kRight) ||
      (ui.input().held(ii::ui::key::kRight) && enter_r_ % 5 == 0 && enter_r_ > 5)) {
    enter_char_ = (enter_char_ + 1) % kAllowedChars.length();
    ui.play_sound(ii::sound::kMenuClick);
  }
  if (ui.input().pressed(ii::ui::key::kLeft) ||
      (ui.input().held(ii::ui::key::kLeft) && enter_r_ % 5 == 0 && enter_r_ > 5)) {
    enter_char_ = (enter_char_ + kAllowedChars.length() - 1) % kAllowedChars.length();
    ui.play_sound(ii::sound::kMenuClick);
  }

  if (ui.input().pressed(ii::ui::key::kMenu)) {
    ui.play_sound(ii::sound::kMenuAccept);
    ui.save_game().high_scores.add_score(results_.mode, results_.players.size() - 1, enter_name_,
                                         get_score());
    if (replay_writer_) {
      ui.write_replay(*replay_writer_, enter_name_, get_score());
    }
    ui.write_save_game();
    quit();
  }
}

void HighScoreModal::render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const {
  std::int32_t players = results_.players.size();
  if (is_high_score(ui.save_game())) {
    render_panel(r, {3.f, 20.f}, {28.f, 27.f});
    render_text(r, {4.f, 21.f}, "It's a high score!", z0Game::kPanelText);
    render_text(r, {4.f, 23.f},
                players == 1 ? "Enter name:" : "Enter team name:", z0Game::kPanelText);
    render_text(r, {6.f, 25.f}, enter_name_, z0Game::kPanelText);
    if ((enter_time_ / 16) % 2 && enter_name_.length() < ii::HighScores::kMaxNameLength) {
      render_text(r, {6.f + enter_name_.length(), 25.f}, kAllowedChars.substr(enter_char_, 1),
                  0xbbbbbbff);
    }
    fvec2 low{4 * kTextWidth + 4, 25 * kTextHeight + 4};
    fvec2 hi{5 * kTextWidth - 4, 26 * kTextHeight - 4};
    render_rect(r, low, hi, z0Game::kPanelText, 1);
  }

  if (results_.mode == ii::game_mode::kBoss) {
    std::int32_t extra_lives = results_.lives_remaining;
    bool b = extra_lives > 0 && results_.killed_bosses >= 6;

    long score = results_.elapsed_time;
    if (b) {
      score -= 10 * extra_lives;
    }
    if (score <= 0) {
      score = 1;
    }

    render_panel(r, {3.f, 3.f}, {37.f, b ? 10.f : 8.f});
    if (b) {
      std::stringstream ss;
      ss << (extra_lives * 10) << "-second extra-life bonus!";
      render_text(r, {4.f, 4.f}, ss.str(), z0Game::kPanelText);
    }

    render_text(r, {4.f, b ? 6.f : 4.f}, "TIME ELAPSED: " + convert_to_time(score),
                z0Game::kPanelText);
    std::stringstream ss;
    ss << "BOSS DESTROY: " << results_.killed_bosses;
    render_text(r, {4.f, b ? 8.f : 6.f}, ss.str(), z0Game::kPanelText);
    return;
  }

  render_panel(r, {3.f, 3.f}, {37.f, 8.f + 2 * players + (players > 1 ? 2 : 0)});

  std::stringstream ss;
  ss << get_score();
  std::string score = ss.str();
  if (score.length() > ii::HighScores::kMaxScoreLength) {
    score = score.substr(0, ii::HighScores::kMaxScoreLength);
  }
  render_text(r, {4.f, 4.f}, "TOTAL SCORE: " + score, z0Game::kPanelText);

  for (std::int32_t i = 0; i < players; ++i) {
    std::stringstream ss;
    if (timer_ % 600 < 300) {
      ss << results_.players[i].score;
    } else {
      auto deaths = results_.players[i].deaths;
      ss << deaths << " death" << (deaths != 1 ? "s" : "");
    }
    score = ss.str();
    if (score.length() > ii::HighScores::kMaxScoreLength) {
      score = score.substr(0, ii::HighScores::kMaxScoreLength);
    }

    ss.str({});
    ss << "PLAYER " << (i + 1) << ":";
    render_text(r, {4.f, 8.f + 2 * i}, ss.str(), z0Game::kPanelText);
    render_text(r, {14.f, 8.f + 2 * i}, score, ii::SimInterface::player_colour(i));
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
    render_text(r, {4.f, 8.f + 2 * players}, s.str(), ii::SimInterface::player_colour(best));

    std::string compliment = kCompliments[compliment_];
    render_text(r, {12.f, 8.f + 2 * players}, compliment, z0Game::kPanelText);
  } else {
    render_text(r, {4.f, 8.f + 2 * players}, "Oh dear!", z0Game::kPanelText);
  }
}

std::int64_t HighScoreModal::get_score() const {
  if (results_.mode == ii::game_mode::kBoss) {
    bool won = results_.killed_bosses >= 6 && results_.elapsed_time != 0;
    return !won ? 0l : std::max(1l, results_.elapsed_time - 600l * results_.lives_remaining);
  }
  std::int64_t total = 0;
  for (const auto& p : results_.players) {
    total += p.score;
  }
  return total;
}

bool HighScoreModal::is_high_score(const ii::SaveGame& save) const {
  return !is_replay_ &&
      save.high_scores.is_high_score(results_.mode, results_.players.size() - 1, get_score());
}

GameModal::GameModal(ii::io::IoLayer& io_layer, const ii::initial_conditions& conditions)
: Modal{true, true} {
  game_.emplace(io_layer, ii::ReplayWriter{conditions});
  game_->input.set_player_count(conditions.player_count);
  game_->input.set_game_dimensions({kWidth, kHeight});
  state_ = std::make_unique<ii::SimState>(conditions, game_->input);
}

GameModal::GameModal(ii::ReplayReader&& replay) : Modal{true, true} {
  auto conditions = replay.initial_conditions();
  replay_.emplace(std::move(replay));
  state_ = std::make_unique<ii::SimState>(conditions, replay_->input);
}

GameModal::~GameModal() {}

void GameModal::update(ii::ui::UiLayer& ui) {
  if (pause_output_ == PauseModal::kEndGame || state_->game_over()) {
    add(std::make_unique<HighScoreModal>(replay_.has_value(), *this, state_->get_results(),
                                         game_ ? &game_->writer : nullptr));
    if (pause_output_ != PauseModal::kEndGame) {
      ui.play_sound(ii::sound::kMenuAccept);
    }
    quit();
    return;
  }

  if (ui.input().pressed(ii::ui::key::kMenu) && !controllers_dialog_) {
    ui.io_layer().capture_mouse(false);
    add(std::make_unique<PauseModal>(&pause_output_));
    ui.play_sound(ii::sound::kMenuAccept);
    return;
  }
  ui.io_layer().capture_mouse(true);

  if (show_controllers_dialog_ && !replay_) {
    controllers_dialog_ = true;
  }
  show_controllers_dialog_ = false;

  if (controllers_dialog_) {
    if (ui.input().pressed(ii::ui::key::kMenu) || ui.input().pressed(ii::ui::key::kAccept)) {
      controllers_dialog_ = false;
      ui.play_sound(ii::sound::kMenuAccept);
      ui.rumble(10);
    }
    return;
  }

  auto frames = state_->frame_count() * frame_count_multiplier_;
  for (std::int32_t i = 0; i < frames; ++i) {
    state_->update();
  }
  auto frame_x = static_cast<std::int32_t>(std::log2(frame_count_multiplier_));
  if (audio_tick_++ % (4 * (1 + frame_x / 2)) == 0) {
    for (const auto& pair : state_->get_sound_output()) {
      auto& s = pair.second;
      ui.play_sound(pair.first, s.volume, s.pan, s.pitch);
    }
    for (const auto& pair : state_->get_rumble_output()) {
      // TODO
    }
    state_->clear_output();
  }

  if (replay_) {
    if (ui.input().pressed(ii::ui::key::kCancel)) {
      frame_count_multiplier_ *= 2;
    }
    if (ui.input().pressed(ii::ui::key::kAccept) && frame_count_multiplier_ > 1) {
      frame_count_multiplier_ /= 2;
    }
  }
}

void GameModal::render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const {
  state_->render();
  auto render = state_->get_render_output();
  r.set_dimensions(ui.io_layer().dimensions(), glm::uvec2{kWidth, kHeight});
  r.set_colour_cycle(render.colour_cycle);
  render_lines(r, render.lines);

  std::int32_t n = 0;
  for (const auto& p : render.players) {
    std::stringstream ss;
    ss << p.multiplier << "X";
    std::string s = ss.str();
    fvec2 v = n == 1 ? fvec2{kWidth / 16 - 1.f - s.length(), 1.f}
        : n == 2     ? fvec2{1.f, kHeight / 16 - 2.f}
        : n == 3     ? fvec2{kWidth / 16 - 1.f - s.length(), kHeight / 16 - 2.f}
                     : fvec2{1.f, 1.f};
    render_text(r, v, s, z0Game::kPanelText);

    ss.str("");
    n % 2 ? ss << p.score << "   " : ss << "   " << p.score;
    render_text(
        r, v - (n % 2 ? fvec2{static_cast<float>(ss.str().length() - s.length()), 0} : fvec2{}),
        ss.str(), p.colour);

    if (p.timer) {
      v.x += n % 2 ? -1 : ss.str().length();
      v *= 16;
      auto lo = v + fvec2{5.f, 11.f - 10 * p.timer};
      auto hi = v + fvec2{9.f, 13.f};
      render_line(r, lo, {lo.x, hi.y}, 0xffffffff);
      render_line(r, {lo.x, hi.y}, hi, 0xffffffff);
      render_line(r, hi, {hi.x, lo.y}, 0xffffffff);
      render_line(r, {hi.x, lo.y}, lo, 0xffffffff);
    }
    ++n;
  }

  if (controllers_dialog_) {
    render_panel(r, {3.f, 3.f}, {32.f, 8.f + 2 * render.players.size()});
    render_text(r, {4.f, 4.f}, "CONTROLLERS FOUND", z0Game::kPanelText);

    for (std::size_t i = 0; i < render.players.size(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      render_text(r, {4.f, 8.f + 2 * i}, ss.str(), z0Game::kPanelText);

      ss.str({});
      bool b = false;
      if (replay_) {
        ss << "REPLAY";
        b = true;
      } else if (game_) {
        auto input_type = game_->input.input_type_for(i);
        if (!input_type) {
          ss << "NONE";
        }
        if (input_type & ii::IoInputAdapter::kController) {
          ss << "GAMEPAD";
          if (input_type & ii::IoInputAdapter::kKeyboardMouse) {
            ss << ", KB/MOUSE";
          }
          b = true;
        } else if (input_type & ii::IoInputAdapter::kKeyboardMouse) {
          ss << "MOUSE & KEYBOARD";
          b = true;
        }
      }

      render_text(r, {14.f, 8.f + 2 * i}, ss.str(),
                  b ? ii::SimInterface::player_colour(i) : z0Game::kPanelText);
    }
    return;
  }

  std::stringstream ss;
  ss << render.lives_remaining << " live(s)";
  if (render.mode != ii::game_mode::kBoss && render.overmind_timer >= 0) {
    auto t = static_cast<std::int32_t>(0.5f + render.overmind_timer / 60);
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  render_text(r, {kWidth / (2.f * kTextWidth) - ss.str().length() / 2, kHeight / kTextHeight - 2.f},
              ss.str(), z0Game::kPanelTran);

  if (render.mode == ii::game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(render.elapsed_time);
    render_text(r, {kWidth / (2 * kTextWidth) - ss.str().length() - 1.f, 1.f}, ss.str(),
                z0Game::kPanelTran);
  }

  if (render.boss_hp_bar) {
    std::int32_t x = render.mode == ii::game_mode::kBoss ? 48 : 0;
    render_rect(r, {x + kWidth / 2 - 48.f, 16.f}, {x + kWidth / 2 + 48.f, 32.f}, z0Game::kPanelTran,
                2);

    render_rect(r, {x + kWidth / 2 - 44.f, 16.f + 4},
                {x + kWidth / 2 - 44.f + 88.f * *render.boss_hp_bar, 32.f - 4}, z0Game::kPanelTran,
                4);
  }

  if (replay_) {
    auto progress = static_cast<float>(replay_->reader.current_input_frame()) /
        replay_->reader.total_input_frames();
    ss.str({});
    ss << frame_count_multiplier_ << "X " << static_cast<std::int32_t>(100 * progress) << "%";

    render_text(r,
                {kWidth / (2.f * kTextWidth) - ss.str().length() / 2, kHeight / kTextHeight - 3.f},
                ss.str(), z0Game::kPanelTran);
  }
}

z0Game::z0Game() : Modal{true, true} {}

void z0Game::update(ii::ui::UiLayer& ui) {
  if (exit_timer_) {
    exit_timer_--;
    quit();
  }
  ui.io_layer().capture_mouse(false);

  menu t = menu_select_;
  if (ui.input().pressed(ii::ui::key::kUp)) {
    menu_select_ = static_cast<menu>(
        std::max(static_cast<std::int32_t>(menu_select_) - 1,
                 static_cast<std::int32_t>(mode_unlocked(ui.save_game()) >= ii::game_mode::kBoss
                                               ? menu::kSpecial
                                               : menu::kStart)));
  }
  if (ui.input().pressed(ii::ui::key::kDown)) {
    menu_select_ = static_cast<menu>(std::min(static_cast<std::int32_t>(menu::kQuit),
                                              static_cast<std::int32_t>(menu_select_) + 1));
  }
  if (t != menu_select_) {
    ui.play_sound(ii::sound::kMenuClick);
  }

  if (menu_select_ == menu::kPlayers) {
    std::int32_t t = player_select_;
    if (ui.input().pressed(ii::ui::key::kLeft)) {
      player_select_ = std::max(1, player_select_ - 1);
    }
    if (ui.input().pressed(ii::ui::key::kRight)) {
      player_select_ = std::min(ii::kMaxPlayers, player_select_ + 1);
    }
    if (ui.input().pressed(ii::ui::key::kAccept) || ui.input().pressed(ii::ui::key::kMenu)) {
      player_select_ = 1 + player_select_ % ii::kMaxPlayers;
    }
    if (t != player_select_) {
      ui.play_sound(ii::sound::kMenuClick);
    }
  }

  if (menu_select_ == menu::kSpecial) {
    ii::game_mode t = mode_select_;
    if (ui.input().pressed(ii::ui::key::kLeft)) {
      mode_select_ =
          static_cast<ii::game_mode>(std::max(static_cast<std::int32_t>(ii::game_mode::kBoss),
                                              static_cast<std::int32_t>(mode_select_) - 1));
    }
    if (ui.input().pressed(ii::ui::key::kRight)) {
      mode_select_ = static_cast<ii::game_mode>(
          std::min(static_cast<std::int32_t>(mode_unlocked(ui.save_game())),
                   static_cast<std::int32_t>(mode_select_) + 1));
    }
    if (t != mode_select_) {
      ui.play_sound(ii::sound::kMenuClick);
    }
  }

  ii::initial_conditions conditions;
  conditions.seed = static_cast<std::int32_t>(time(0));
  conditions.can_face_secret_boss = mode_unlocked(ui.save_game()) >= ii::game_mode::kFast;
  conditions.player_count = player_select_;
  conditions.mode = ii::game_mode::kNormal;

  if (ui.input().pressed(ii::ui::key::kAccept) || ui.input().pressed(ii::ui::key::kMenu)) {
    if (menu_select_ == menu::kStart) {
      add(std::make_unique<GameModal>(ui.io_layer(), conditions));
    } else if (menu_select_ == menu::kQuit) {
      exit_timer_ = 2;
    } else if (menu_select_ == menu::kSpecial) {
      conditions.mode = mode_select_;
      add(std::make_unique<GameModal>(ui.io_layer(), conditions));
    }
    if (menu_select_ != menu::kPlayers) {
      ui.play_sound(ii::sound::kMenuAccept);
    }
  }
}

void z0Game::render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const {
  if (menu_select_ >= menu::kStart || mode_select_ == ii::game_mode::kBoss) {
    r.set_colour_cycle(0);
  } else if (mode_select_ == ii::game_mode::kHard) {
    r.set_colour_cycle(128);
  } else if (mode_select_ == ii::game_mode::kFast) {
    r.set_colour_cycle(192);
  } else if (mode_select_ == ii::game_mode::kWhat) {
    r.set_colour_cycle((r.colour_cycle() + 1) % 256);
  }

  r.set_dimensions(ui.io_layer().dimensions(), glm::uvec2{kWidth, kHeight});
  render_panel(r, {3.f, 3.f}, {19.f, 14.f});

  render_text(r, {37.f - 16, 3.f}, "coded by: SEiKEN", kPanelText);
  render_text(r, {37.f - 16, 4.f}, "stu@seiken.co.uk", kPanelText);
  render_text(r, {37.f - 9, 6.f}, "-testers-", kPanelText);
  render_text(r, {37.f - 9, 7.f}, "MATT BELL", kPanelText);
  render_text(r, {37.f - 9, 8.f}, "RUFUZZZZZ", kPanelText);
  render_text(r, {37.f - 9, 9.f}, "SHADOW1W2", kPanelText);

  std::string b = "BOSSES:  ";
  std::int32_t bb = mode_unlocked(ui.save_game()) >= ii::game_mode::kHard
      ? ui.save_game().hard_mode_bosses_killed
      : ui.save_game().bosses_killed;
  b += bb & ii::SimInterface::kBoss1A ? "X" : "-";
  b += bb & ii::SimInterface::kBoss1B ? "X" : "-";
  b += bb & ii::SimInterface::kBoss1C ? "X" : "-";
  b += bb & ii::SimInterface::kBoss3A ? "X" : " ";
  b += bb & ii::SimInterface::kBoss2A ? "X" : "-";
  b += bb & ii::SimInterface::kBoss2B ? "X" : "-";
  b += bb & ii::SimInterface::kBoss2C ? "X" : "-";
  render_text(r, {37.f - 16, 13.f}, b, kPanelText);

  render_text(r, {4.f, 4.f}, "WiiSPACE", kPanelText);
  render_text(r, {6.f, 8.f}, "START GAME", kPanelText);
  render_text(r, {6.f, 10.f}, "PLAYERS", kPanelText);
  render_text(r, {6.f, 12.f}, "EXiT", kPanelText);

  if (mode_unlocked(ui.save_game()) >= ii::game_mode::kBoss) {
    std::string str = mode_select_ == ii::game_mode::kBoss ? "BOSS MODE"
        : mode_select_ == ii::game_mode::kHard             ? "HARD MODE"
        : mode_select_ == ii::game_mode::kFast             ? "FAST MODE"
                                                           : "W-HAT MODE";
    render_text(r, {6.f, 6.f}, str, kPanelText);
  }
  if (menu_select_ == menu::kSpecial && mode_select_ > ii::game_mode::kBoss) {
    render_text(r, {5.f, 6.f}, "<", kPanelTran);
  }
  if (menu_select_ == menu::kSpecial && mode_select_ < mode_unlocked(ui.save_game())) {
    render_text(r, {6.f, 6.f}, "         >", kPanelTran);
  }

  fvec2 low{
      static_cast<float>(4 * kTextWidth + 4),
      static_cast<float>((6 + 2 * static_cast<std::int32_t>(menu_select_)) * kTextHeight + 4)};
  fvec2 hi{static_cast<float>(5 * kTextWidth - 4),
           static_cast<float>((7 + 2 * static_cast<std::int32_t>(menu_select_)) * kTextHeight - 4)};
  render_rect(r, low, hi, kPanelText, 1);

  if (player_select_ > 1 && menu_select_ == menu::kPlayers) {
    render_text(r, {5.f, 10.f}, "<", kPanelTran);
  }
  if (player_select_ < 4 && menu_select_ == menu::kPlayers) {
    render_text(r, {14.f + player_select_, 10.f}, ">", kPanelTran);
  }
  for (std::int32_t i = 0; i < player_select_; ++i) {
    std::stringstream ss;
    ss << (i + 1);
    render_text(r, {14.f + i, 10.f}, ss.str(), ii::SimInterface::player_colour(i));
  }

  render_panel(r, {3.f, 15.f}, {37.f, 27.f});

  std::stringstream ss;
  ss << player_select_;
  std::string s = "HiGH SCORES    ";
  s += menu_select_ == menu::kSpecial
      ? (mode_select_ == ii::game_mode::kBoss       ? "BOSS MODE"
             : mode_select_ == ii::game_mode::kHard ? "HARD MODE (" + ss.str() + "P)"
             : mode_select_ == ii::game_mode::kFast ? "FAST MODE (" + ss.str() + "P)"
                                                    : "W-HAT MODE (" + ss.str() + "P)")
      : player_select_ == 1 ? "ONE PLAYER"
      : player_select_ == 2 ? "TWO PLAYERS"
      : player_select_ == 3 ? "THREE PLAYERS"
      : player_select_ == 4 ? "FOUR PLAYERS"
                            : "";
  render_text(r, {4.f, 16.f}, s, kPanelText);

  if (menu_select_ == menu::kSpecial && mode_select_ == ii::game_mode::kBoss) {
    render_text(r, {4.f, 18.f}, "ONE PLAYER", kPanelText);
    render_text(r, {4.f, 20.f}, "TWO PLAYERS", kPanelText);
    render_text(r, {4.f, 22.f}, "THREE PLAYERS", kPanelText);
    render_text(r, {4.f, 24.f}, "FOUR PLAYERS", kPanelText);

    for (std::size_t i = 0; i < ii::kMaxPlayers; ++i) {
      auto& s = ui.save_game().high_scores.get(ii::game_mode::kBoss, i, 0);
      std::string score = convert_to_time(s.score).substr(0, ii::HighScores::kMaxNameLength);
      std::string name = s.name.substr(0, ii::HighScores::kMaxNameLength);

      render_text(r, {19.f, 18.f + i * 2}, score, kPanelText);
      render_text(r, {19.f, 19.f + i * 2}, name, kPanelText);
    }
  } else {
    for (std::size_t i = 0; i < ii::HighScores::kNumScores; ++i) {
      ss.str({});
      ss << (i + 1) << ".";
      render_text(r, {4.f, 18.f + i}, ss.str(), kPanelText);

      auto& s = ui.save_game().high_scores.get(
          menu_select_ == menu::kSpecial ? mode_select_ : ii::game_mode::kNormal,
          player_select_ - 1, i);
      if (s.score <= 0) {
        continue;
      }

      ss.str({});
      ss << s.score;
      std::string score = ss.str().substr(0, ii::HighScores::kMaxScoreLength);
      std::string name = s.name.substr(0, ii::HighScores::kMaxScoreLength);

      render_text(r, {7.f, 18.f + i}, score, kPanelText);
      render_text(r, {19.f, 18.f + i}, name, kPanelText);
    }
  }
}

ii::game_mode z0Game::mode_unlocked(const ii::SaveGame& save) const {
  if (!(save.bosses_killed & 63)) {
    return ii::game_mode::kNormal;
  }
  if (save.high_scores.boss[0].score == 0 && save.high_scores.boss[1].score == 0 &&
      save.high_scores.boss[2].score == 0 && save.high_scores.boss[3].score == 0) {
    return ii::game_mode::kBoss;
  }
  if ((save.hard_mode_bosses_killed & 63) != 63) {
    return ii::game_mode::kHard;
  }
  if ((save.hard_mode_bosses_killed & 64) != 64) {
    return ii::game_mode::kFast;
  }
  return ii::game_mode::kWhat;
}
