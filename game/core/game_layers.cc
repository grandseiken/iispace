#include "game/core/game_layers.h"
#include "game/core/game_stack.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <sstream>

namespace ii {
namespace {
// TODO: used for some stuff that should use value from sim state instead.
constexpr glm::uvec2 kDimensions = {640, 480};
constexpr glm::uvec2 kTextSize = {16, 16};

constexpr glm::vec4 kPanelText = {0.f, 0.f, .925f, 1.f};
constexpr glm::vec4 kPanelTran = {0.f, 0.f, .925f, .6f};
constexpr glm::vec4 kPanelBack = {0.f, 0.f, 0.f, 1.f};

std::string convert_to_time(std::uint64_t score) {
  if (score == 0) {
    return "--:--";
  }
  std::uint64_t mins = 0;
  while (score >= 60 * 60 && mins < 99) {
    score -= 60 * 60;
    ++mins;
  }
  std::uint64_t secs = score / 60;

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

void render_text(render::GlRenderer& r, const glm::vec2& v, const std::string& text,
                 const glm::vec4& c) {
  r.render_text(0, 16 * static_cast<glm::ivec2>(v), c, ustring_view::utf8(text));
}

}  // namespace

PauseLayer::PauseLayer(ui::GameStack& stack, output_t* output)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate}, output_{output} {
  *output = kContinue;
}

void PauseLayer::update(const ui::input_frame& input) {
  auto t = selection_;
  if (input.pressed(ui::key::kUp)) {
    selection_ = std::max(0u, selection_ - 1);
  }
  if (input.pressed(ui::key::kDown)) {
    selection_ = std::min(2u, selection_ + 1);
  }
  if (t != selection_) {
    stack().play_sound(sound::kMenuClick);
  }

  bool accept = input.pressed(ui::key::kAccept) || input.pressed(ui::key::kMenu);
  if (accept && selection_ == 0) {
    close();
  } else if (accept && selection_ == 1) {
    *output_ = kEndGame;
    close();
  } else if (accept && selection_ == 2) {
    stack().write_config();
    stack().set_volume(stack().config().volume);
  }
  if (accept) {
    stack().play_sound(sound::kMenuAccept);
  }

  if (input.pressed(ui::key::kCancel)) {
    close();
    stack().play_sound(sound::kMenuAccept);
  }

  auto v = stack().config().volume;
  if (selection_ == 2 && input.pressed(ui::key::kLeft)) {
    stack().config().volume = std::max(0.f, stack().config().volume - 1.f);
  }
  if (selection_ == 2 && input.pressed(ui::key::kRight)) {
    stack().config().volume = std::min(100.f, stack().config().volume + 1.f);
  }
  if (v != stack().config().volume) {
    stack().set_volume(stack().config().volume);
    stack().write_config();
    stack().play_sound(sound::kMenuClick);
  }
}

void PauseLayer::render(render::GlRenderer& r) const {
  render_text(r, {4.f, 4.f}, "PAUSED", kPanelText);
  render_text(r, {6.f, 8.f}, "CONTINUE", kPanelText);
  render_text(r, {6.f, 10.f}, "END GAME", kPanelText);
  render_text(r, {6.f, 12.f}, "VOL.", kPanelText);
  std::stringstream vol;
  auto v = static_cast<std::uint32_t>(stack().config().volume);
  vol << " " << (v < 10 ? " " : "") << v;
  render_text(r, {10.f, 12.f}, vol.str(), kPanelText);
  if (selection_ == 2 && v > 0) {
    render_text(r, {5.f, 12.f}, "<", kPanelTran);
  }
  if (selection_ == 2 && v < 100) {
    render_text(r, {13.f, 12.f}, ">", kPanelTran);
  }
}

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   const game_options_t& options)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender}
, engine_{std::random_device{}()}
, options_{options}
, mode_{conditions.mode}
, render_state_{conditions.seed} {
  game_.emplace(stack.io_layer(), data::ReplayWriter{conditions});
  game_->input.set_player_count(conditions.player_count);
  game_->input.set_game_dimensions(kDimensions);
  render_state_.set_dimensions(kDimensions);
  state_ = std::make_unique<SimState>(conditions, &game_->writer, options.ai_players);
}

SimLayer::SimLayer(ui::GameStack& stack, data::ReplayReader&& replay, const game_options_t& options)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender}
, engine_{std::random_device{}()}
, options_{options}
, mode_{replay.initial_conditions().mode}
, render_state_{replay.initial_conditions().seed} {
  auto conditions = replay.initial_conditions();
  replay_.emplace(std::move(replay));
  render_state_.set_dimensions(kDimensions);
  if (options.replay_remote_players.empty()) {
    state_ = std::make_unique<SimState>(conditions);
  } else {
    NetworkedSimState::input_mapping mapping;
    for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
      if (std::find(options.replay_remote_players.begin(), options.replay_remote_players.end(),
                    i) == options.replay_remote_players.end()) {
        mapping.local.player_numbers.emplace_back(i);
      } else {
        mapping.remote["remote"].player_numbers.emplace_back(i);
      }
    }
    network_state_ = std::make_unique<NetworkedSimState>(conditions, mapping);
  }
}

SimLayer::~SimLayer() = default;

void SimLayer::update(const ui::input_frame& input) {
  auto& istate = network_state_ ? static_cast<ISimState&>(*network_state_) : *state_;
  stack().set_fps((network_state_ ? static_cast<ISimState&>(*network_state_) : *state_).fps());

  if (pause_output_ == PauseLayer::kEndGame || istate.game_over()) {
    if (!replay_) {
      if (mode_ == game_mode::kNormal || mode_ == game_mode::kBoss) {
        stack().savegame().bosses_killed |= istate.results().bosses_killed();
      } else {
        stack().savegame().hard_mode_bosses_killed |= istate.results().bosses_killed();
      }
      stack().write_savegame();
      if (game_) {
        stack().write_replay(game_->writer, "untitled", istate.results().score);
      }
    }
    if (pause_output_ != PauseLayer::kEndGame) {
      stack().play_sound(sound::kMenuAccept);
    }
    close();
    return;
  }

  if (show_controllers_dialog_ && !replay_) {
    controllers_dialog_ = true;
  }
  show_controllers_dialog_ = false;

  if (controllers_dialog_) {
    if (input.pressed(ui::key::kMenu) || input.pressed(ui::key::kAccept)) {
      controllers_dialog_ = false;
      stack().play_sound(sound::kMenuAccept);
      stack().rumble(10);
    }
    return;
  }

  for (std::uint32_t i = 0; i < frame_count_multiplier_; ++i) {
    if (state_) {
      auto input = game_ ? game_->input.get() : replay_->reader.next_tick_input_frames();
      if (game_) {
        state_->ai_think(input);
      }
      state_->update(input);
    } else {
      auto frames = replay_->reader.next_tick_input_frames();
      std::vector<input_frame> local_frames;
      replay_network_packet packet;
      for (std::uint32_t i = 0; i < replay_->reader.initial_conditions().player_count; ++i) {
        if (std::find(options_.replay_remote_players.begin(), options_.replay_remote_players.end(),
                      i) != options_.replay_remote_players.end()) {
          packet.packet.input_frames.emplace_back(frames[i]);
        } else {
          local_frames.emplace_back(frames[i]);
        }
      }
      packet.packet.canonical_checksum = 0;
      auto current_tick = istate.tick_count();
      packet.packet.canonical_tick_count = packet.packet.tick_count = current_tick;

      std::uniform_int_distribution<std::uint64_t> d{options_.replay_min_tick_delivery_delay,
                                                     options_.replay_max_tick_delivery_delay};
      packet.delivery_tick_count = current_tick + d(engine_);
      replay_packets_.emplace_back(packet);

      auto end = std::find_if(replay_packets_.begin(), replay_packets_.end(),
                              [&](const auto& p) { return p.delivery_tick_count > current_tick; });
      for (auto it = replay_packets_.begin(); it != end; ++it) {
        network_state_->input_packet("remote", it->packet);
      }
      replay_packets_.erase(replay_packets_.begin(), end);
      network_state_->update(local_frames);
    }
  }

  auto frame_x = static_cast<std::uint32_t>(std::log2(frame_count_multiplier_));
  bool handle_audio = !(audio_tick_++ % (4 * (1 + frame_x / 2)));
  bool handle_rumble = game_ && !replay_;
  render_state_.handle_output(istate, handle_audio ? &stack().mixer() : nullptr,
                              handle_rumble ? &game_->input : nullptr);
  render_state_.update(handle_rumble ? &game_->input : nullptr);

  if (replay_) {
    if (input.pressed(ui::key::kCancel)) {
      frame_count_multiplier_ *= 2;
    }
    if (input.pressed(ui::key::kAccept) && frame_count_multiplier_ > 1) {
      frame_count_multiplier_ /= 2;
    }
  }

  if (input.pressed(ui::key::kMenu) && !controllers_dialog_) {
    stack().io_layer().capture_mouse(false);
    stack().add<PauseLayer>(&pause_output_);
    stack().play_sound(sound::kMenuAccept);
    return;
  }
  stack().io_layer().capture_mouse(true);
}

void SimLayer::render(render::GlRenderer& r) const {
  auto& istate = network_state_ ? static_cast<ISimState&>(*network_state_) : *state_;
  const auto& render = istate.render(/* paused */ controllers_dialog_ || stack().top() != this);
  r.set_dimensions(stack().io_layer().dimensions(), kDimensions);
  r.set_colour_cycle(render.colour_cycle);
  render_state_.render(r);  // TODO: can be merged with below?
  r.render_shapes(render.shapes,
                  /* trail alpha */ 1.f / (1.f + std::log2f(frame_count_multiplier_)));

  std::uint32_t n = 0;
  for (const auto& p : render.players) {
    std::stringstream ss;
    ss << p.multiplier << "X";
    std::string s = ss.str();
    auto v = n == 1 ? glm::vec2{kDimensions.x / 16 - 1.f - s.size(), 1.f}
        : n == 2    ? glm::vec2{1.f, kDimensions.y / 16 - 2.f}
        : n == 3    ? glm::vec2{kDimensions.x / 16 - 1.f - s.size(), kDimensions.y / 16 - 2.f}
                    : glm::vec2{1.f, 1.f};
    render_text(r, v, s, kPanelText);

    ss.str("");
    n % 2 ? ss << p.score << "   " : ss << "   " << p.score;
    render_text(
        r,
        v - (n % 2 ? glm::vec2{static_cast<float>(ss.str().size() - s.size()), 0} : glm::vec2{0.f}),
        ss.str(), p.colour);

    if (p.timer) {
      v.x += n % 2 ? -1 : ss.str().size();
      v *= 16;
      auto lo = v + glm::vec2{5.f, 11.f - 10 * p.timer};
      auto hi = v + glm::vec2{9.f, 13.f};
      std::vector<render::shape> shapes{render::shape{
          .origin = (lo + hi) / 2.f,
          .colour = glm::vec4{1.f},
          .data = render::box{.dimensions = (hi - lo) / 2.f},
      }};
      r.render_shapes(shapes, 1.f);
    }
    ++n;
  }

  if (controllers_dialog_) {
    render_text(r, {4.f, 4.f}, "CONTROLLERS FOUND", kPanelText);

    for (std::size_t i = 0; i < render.players.size(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      render_text(r, {4.f, 8.f + 2 * i}, ss.str(), kPanelText);

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
        if (input_type & IoInputAdapter::kController) {
          ss << "GAMEPAD";
          if (input_type & IoInputAdapter::kKeyboardMouse) {
            ss << ", KB/MOUSE";
          }
          b = true;
        } else if (input_type & IoInputAdapter::kKeyboardMouse) {
          ss << "MOUSE & KEYBOARD";
          b = true;
        }
      }

      render_text(r, {14.f, 8.f + 2 * i}, ss.str(), b ? player_colour(i) : kPanelText);
    }
    return;
  }

  std::stringstream ss;
  ss << render.lives_remaining << " live(s)";
  if (mode_ != game_mode::kBoss && render.overmind_timer) {
    auto t = *render.overmind_timer / 60;
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  render_text(r,
              {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
               kDimensions.y / kTextSize.y - 2.f},
              ss.str(), kPanelTran);

  if (mode_ == game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(render.tick_count);
    render_text(r, {kDimensions.x / (2 * kTextSize.x) - ss.str().size() - 1.f, 1.f}, ss.str(),
                kPanelTran);
  }

  if (render.boss_hp_bar) {
    std::uint32_t x = mode_ == game_mode::kBoss ? 48 : 0;
  }

  if (replay_) {
    auto progress = static_cast<float>(replay_->reader.current_input_frame()) /
        replay_->reader.total_input_frames();
    ss.str({});
    ss << frame_count_multiplier_ << "X " << static_cast<std::uint32_t>(100 * progress) << "%";

    render_text(r,
                {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
                 kDimensions.y / kTextSize.y - 3.f},
                ss.str(), kPanelTran);
  }
}

MainMenuLayer::MainMenuLayer(ui::GameStack& stack, const game_options_t& options)
: ui::GameLayer{stack}, options_{options} {}

void MainMenuLayer::update(const ui::input_frame& input) {
  if (exit_timer_) {
    exit_timer_--;
    close();
  }
  stack().set_fps(60);
  stack().io_layer().capture_mouse(false);

  menu t = menu_select_;
  if (input.pressed(ui::key::kUp)) {
    menu_select_ = static_cast<menu>(std::max(static_cast<std::uint32_t>(menu_select_) - 1,
                                              static_cast<std::uint32_t>(menu::kSpecial)));
  }
  if (input.pressed(ui::key::kDown)) {
    menu_select_ = static_cast<menu>(std::min(static_cast<std::uint32_t>(menu::kQuit),
                                              static_cast<std::uint32_t>(menu_select_) + 1));
  }
  if (t != menu_select_) {
    stack().play_sound(sound::kMenuClick);
  }

  if (menu_select_ == menu::kPlayers) {
    auto t = player_select_;
    if (input.pressed(ui::key::kLeft)) {
      player_select_ = std::max(1u, player_select_ - 1);
    }
    if (input.pressed(ui::key::kRight)) {
      player_select_ = std::min(kMaxPlayers, player_select_ + 1);
    }
    if (input.pressed(ui::key::kAccept) || input.pressed(ui::key::kMenu)) {
      player_select_ = 1 + player_select_ % kMaxPlayers;
    }
    if (t != player_select_) {
      stack().play_sound(sound::kMenuClick);
    }
  }

  if (menu_select_ == menu::kSpecial) {
    game_mode t = mode_select_;
    if (input.pressed(ui::key::kLeft)) {
      mode_select_ = static_cast<game_mode>(std::max(static_cast<std::uint32_t>(game_mode::kBoss),
                                                     static_cast<std::uint32_t>(mode_select_) - 1));
    }
    if (input.pressed(ui::key::kRight)) {
      mode_select_ = static_cast<game_mode>(std::min(static_cast<std::uint32_t>(game_mode::kWhat),
                                                     static_cast<std::uint32_t>(mode_select_) + 1));
    }
    if (t != mode_select_) {
      stack().play_sound(sound::kMenuClick);
    }
  }

  initial_conditions conditions;
  conditions.compatibility = options_.compatibility;
  conditions.seed = static_cast<std::uint32_t>(time(0));
  conditions.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
  conditions.player_count = player_select_;
  conditions.mode = game_mode::kNormal;

  if (input.pressed(ui::key::kAccept) || input.pressed(ui::key::kMenu)) {
    if (menu_select_ == menu::kStart) {
      stack().add<SimLayer>(conditions, options_);
    } else if (menu_select_ == menu::kQuit) {
      exit_timer_ = 2;
    } else if (menu_select_ == menu::kSpecial) {
      conditions.mode = mode_select_;
      stack().add<SimLayer>(conditions, options_);
    }
    if (menu_select_ != menu::kPlayers) {
      stack().play_sound(sound::kMenuAccept);
    }
  }
}

void MainMenuLayer::render(render::GlRenderer& r) const {
  if (menu_select_ >= menu::kStart || mode_select_ == game_mode::kBoss) {
    r.set_colour_cycle(0);
  } else if (mode_select_ == game_mode::kHard) {
    r.set_colour_cycle(128);
  } else if (mode_select_ == game_mode::kFast) {
    r.set_colour_cycle(192);
  } else if (mode_select_ == game_mode::kWhat) {
    r.set_colour_cycle((r.colour_cycle() + 1) % 256);
  }

  r.set_dimensions(stack().io_layer().dimensions(), kDimensions);
  render_text(r, {6.f, 8.f}, "START GAME", kPanelText);
  render_text(r, {6.f, 10.f}, "PLAYERS", kPanelText);
  render_text(r, {6.f, 12.f}, "EXiT", kPanelText);

  std::string str = mode_select_ == game_mode::kBoss ? "BOSS MODE"
      : mode_select_ == game_mode::kHard             ? "HARD MODE"
      : mode_select_ == game_mode::kFast             ? "FAST MODE"
                                                     : "W-HAT MODE";
  render_text(r, {6.f, 6.f}, str, kPanelText);
  if (menu_select_ == menu::kSpecial && mode_select_ > game_mode::kBoss) {
    render_text(r, {5.f, 6.f}, "<", kPanelTran);
  }
  if (menu_select_ == menu::kSpecial && mode_select_ < game_mode::kWhat) {
    render_text(r, {6.f, 6.f}, "         >", kPanelTran);
  }

  if (player_select_ > 1 && menu_select_ == menu::kPlayers) {
    render_text(r, {5.f, 10.f}, "<", kPanelTran);
  }
  if (player_select_ < 4 && menu_select_ == menu::kPlayers) {
    render_text(r, {14.f + player_select_, 10.f}, ">", kPanelTran);
  }
  for (std::uint32_t i = 0; i < player_select_; ++i) {
    std::stringstream ss;
    ss << (i + 1);
    render_text(r, {14.f + i, 10.f}, ss.str(), player_colour(i));
  }

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
  render_text(r, {4.f, 16.f}, s, kPanelText);
}

}  // namespace ii