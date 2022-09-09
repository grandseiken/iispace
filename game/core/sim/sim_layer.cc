#include "game/core/sim/sim_layer.h"
#include "game/core/game_options.h"
#include "game/core/sim/input_adapter.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <sstream>

namespace ii {
namespace {
// TODO: rewrite everything in here. Split into SimLayer and ReplayLayer.
// TODO: used for some stuff that should use value from sim state instead.
constexpr glm::uvec2 kDimensions = {640, 480};
constexpr glm::uvec2 kTextSize = {16, 16};

constexpr glm::vec4 kPanelText = {0.f, 0.f, .925f, 1.f};
constexpr glm::vec4 kPanelTran = {0.f, 0.f, .925f, .6f};

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

void render_text(const render::GlRenderer& r, const glm::vec2& v, const std::string& text,
                 const glm::vec4& c) {
  r.render_text(render::font_id::kDefault, {16, 16}, 16 * static_cast<glm::ivec2>(v), c,
                ustring_view::utf8(text));
}

}  // namespace

struct SimLayer::impl_t {
  impl_t(const initial_conditions& conditions, const game_options_t& options)
  : engine{std::random_device{}()}
  , options{options}
  , mode{conditions.mode}
  , render_state{conditions.seed} {}

  struct replay_t {
    replay_t(data::ReplayReader&& r) : reader{std::move(r)} {}
    data::ReplayReader reader;
  };
  struct game_t {
    game_t(io::IoLayer& io, data::ReplayWriter&& w) : writer{std::move(w)}, input{io} {}
    data::ReplayWriter writer;
    InputAdapter input;
  };

  std::uint32_t frame_count_multiplier = 1;
  std::uint32_t audio_tick = 0;
  bool show_controllers_dialog = true;
  bool controllers_dialog = true;

  struct replay_network_packet {
    std::uint64_t delivery_tick_count = 0;
    sim_packet packet;
  };

  std::mt19937_64 engine;
  game_options_t options;
  game_mode mode;
  RenderState render_state;
  std::optional<replay_t> replay;
  std::optional<game_t> game;
  std::vector<replay_network_packet> replay_packets;
  std::unique_ptr<SimState> state;
  std::unique_ptr<NetworkedSimState> network_state;
};

SimLayer::~SimLayer() = default;

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   const game_options_t& options)
: ui::GameLayer{stack,
                ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender |
                    ui::layer_flag::kCaptureCursor}
, impl_{std::make_unique<impl_t>(conditions, options)} {
  impl_->game.emplace(stack.io_layer(), data::ReplayWriter{conditions});
  impl_->game->input.set_player_count(conditions.player_count);
  impl_->state = std::make_unique<SimState>(conditions, &impl_->game->writer, options.ai_players);
  add_flags(ui::element_flags::kCanFocus);
  focus();
}

SimLayer::SimLayer(ui::GameStack& stack, data::ReplayReader&& replay, const game_options_t& options)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender}
, impl_{std::make_unique<impl_t>(replay.initial_conditions(), options)} {
  auto conditions = replay.initial_conditions();
  impl_->replay.emplace(std::move(replay));
  if (options.replay_remote_players.empty()) {
    impl_->state = std::make_unique<SimState>(conditions);
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
    impl_->network_state = std::make_unique<NetworkedSimState>(conditions, mapping);
  }
  add_flags(ui::element_flags::kCanFocus);
  focus();
}

void SimLayer::update_content(const ui::input_frame& input, ui::output_frame&) {
  stack().io_layer().capture_mouse(true);
  auto& istate =
      impl_->network_state ? static_cast<ISimState&>(*impl_->network_state) : *impl_->state;
  set_bounds(rect{istate.dimensions()});
  stack().set_fps(
      (impl_->network_state ? static_cast<ISimState&>(*impl_->network_state) : *impl_->state)
          .fps());

  if (is_removed()) {
    return;
  }
  if (istate.game_over()) {
    end_game();
    stack().play_sound(sound::kMenuAccept);
    remove();
    return;
  }

  if (impl_->show_controllers_dialog && !impl_->replay) {
    impl_->controllers_dialog = true;
  }
  impl_->show_controllers_dialog = false;

  if (impl_->controllers_dialog) {
    if (input.pressed(ui::key::kStart) || input.pressed(ui::key::kAccept)) {
      impl_->controllers_dialog = false;
      stack().play_sound(sound::kMenuAccept);
    }
    return;
  }

  for (std::uint32_t i = 0; i < impl_->frame_count_multiplier; ++i) {
    if (impl_->state) {
      auto input = impl_->game
          ? (impl_->game->input.set_game_dimensions(istate.dimensions()), impl_->game->input.get())
          : impl_->replay->reader.next_tick_input_frames();
      if (impl_->game) {
        impl_->state->ai_think(input);
      }
      impl_->state->update(input);
    } else {
      auto frames = impl_->replay->reader.next_tick_input_frames();
      std::vector<input_frame> local_frames;
      impl_t::replay_network_packet packet;
      for (std::uint32_t i = 0; i < impl_->replay->reader.initial_conditions().player_count; ++i) {
        if (std::find(impl_->options.replay_remote_players.begin(),
                      impl_->options.replay_remote_players.end(),
                      i) != impl_->options.replay_remote_players.end()) {
          packet.packet.input_frames.emplace_back(frames[i]);
        } else {
          local_frames.emplace_back(frames[i]);
        }
      }
      packet.packet.canonical_checksum = 0;
      auto current_tick = istate.tick_count();
      packet.packet.canonical_tick_count = packet.packet.tick_count = current_tick;

      std::uniform_int_distribution<std::uint64_t> d{impl_->options.replay_min_tick_delivery_delay,
                                                     impl_->options.replay_max_tick_delivery_delay};
      packet.delivery_tick_count = current_tick + d(impl_->engine);
      impl_->replay_packets.emplace_back(packet);

      auto end = std::find_if(impl_->replay_packets.begin(), impl_->replay_packets.end(),
                              [&](const auto& p) { return p.delivery_tick_count > current_tick; });
      for (auto it = impl_->replay_packets.begin(); it != end; ++it) {
        impl_->network_state->input_packet("remote", it->packet);
      }
      impl_->replay_packets.erase(impl_->replay_packets.begin(), end);
      impl_->network_state->update(local_frames);
    }
  }

  auto frame_x = static_cast<std::uint32_t>(std::log2(impl_->frame_count_multiplier));
  bool handle_audio = !(impl_->audio_tick++ % (4 * (1 + frame_x / 2)));
  bool handle_rumble = impl_->game && !impl_->replay;
  impl_->render_state.set_dimensions(istate.dimensions());
  impl_->render_state.handle_output(istate, handle_audio ? &stack().mixer() : nullptr,
                                    handle_rumble ? &impl_->game->input : nullptr);
  impl_->render_state.update(handle_rumble ? &impl_->game->input : nullptr);

  if (impl_->replay) {
    if (input.pressed(ui::key::kCancel)) {
      impl_->frame_count_multiplier *= 2;
    }
    if (input.pressed(ui::key::kAccept) && impl_->frame_count_multiplier > 1) {
      impl_->frame_count_multiplier /= 2;
    }
  }

  if ((input.pressed(ui::key::kStart) || input.pressed(ui::key::kEscape)) &&
      !impl_->controllers_dialog) {
    stack().add<PauseLayer>([this] {
      end_game();
      remove();
    });
    return;
  }
}

void SimLayer::render_content(render::GlRenderer& r) const {
  auto& istate =
      impl_->network_state ? static_cast<ISimState&>(*impl_->network_state) : *impl_->state;
  const auto& render =
      istate.render(/* paused */ impl_->controllers_dialog || stack().top() != this);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r);  // TODO: can be merged with below?
  // TODO: instead of messing with trail alpha, render N frames ago first so trails aren't quite so
  // long?
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes,
                  /* trail alpha */ 1.f / (1.f + std::log2f(impl_->frame_count_multiplier)));

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
      r.render_shapes(render::coordinate_system::kGlobal, shapes, 1.f);
    }
    ++n;
  }

  if (impl_->controllers_dialog) {
    render_text(r, {4.f, 4.f}, "CONTROLLERS FOUND", kPanelText);

    for (std::size_t i = 0; i < render.players.size(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      render_text(r, {4.f, 8.f + 2 * i}, ss.str(), kPanelText);

      ss.str({});
      bool b = false;
      if (impl_->replay) {
        ss << "REPLAY";
        b = true;
      } else if (impl_->game) {
        auto input_type = impl_->game->input.input_type_for(i);
        if (!input_type) {
          ss << "NONE";
        }
        if (input_type & InputAdapter::kController) {
          ss << "GAMEPAD";
          if (input_type & InputAdapter::kKeyboardMouse) {
            ss << ", KB/MOUSE";
          }
          b = true;
        } else if (input_type & InputAdapter::kKeyboardMouse) {
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
  if (impl_->mode != game_mode::kBoss && render.overmind_timer) {
    auto t = *render.overmind_timer / 60;
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  render_text(r,
              {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
               kDimensions.y / kTextSize.y - 2.f},
              ss.str(), kPanelTran);

  if (impl_->mode == game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(render.tick_count);
    render_text(r, {kDimensions.x / (2 * kTextSize.x) - ss.str().size() - 1.f, 1.f}, ss.str(),
                kPanelTran);
  }

  if (impl_->replay) {
    auto progress = static_cast<float>(impl_->replay->reader.current_input_frame()) /
        impl_->replay->reader.total_input_frames();
    ss.str({});
    ss << impl_->frame_count_multiplier << "X " << static_cast<std::uint32_t>(100 * progress)
       << "%";

    render_text(r,
                {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
                 kDimensions.y / kTextSize.y - 3.f},
                ss.str(), kPanelTran);
  }
}

void SimLayer::end_game() {
  auto& istate =
      impl_->network_state ? static_cast<ISimState&>(*impl_->network_state) : *impl_->state;
  if (!impl_->replay) {
    if (impl_->mode == game_mode::kNormal || impl_->mode == game_mode::kBoss) {
      stack().savegame().bosses_killed |= istate.results().bosses_killed();
    } else {
      stack().savegame().hard_mode_bosses_killed |= istate.results().bosses_killed();
    }
    stack().write_savegame();
    if (impl_->game) {
      stack().write_replay(impl_->game->writer, "untitled", istate.results().score);
    }
  }
}

}  // namespace ii
