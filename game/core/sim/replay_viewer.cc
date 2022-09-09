#include "game/core/sim/replay_viewer.h"
#include "game/core/game_options.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/core/sim/sim_layer_internal.h"
#include "game/data/replay.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <sstream>

namespace ii {

struct ReplayViewer::impl_t {
  impl_t(data::ReplayReader&& reader, const game_options_t& options)
  : reader{std::move(reader)}
  , options{options}
  , mode{this->reader.initial_conditions().mode}
  , render_state{this->reader.initial_conditions().seed}
  , engine{std::random_device{}()} {}

  data::ReplayReader reader;
  game_options_t options;
  game_mode mode;
  RenderState render_state;
  std::uint32_t frame_count_multiplier = 1;
  std::uint32_t audio_tick = 0;
  std::unique_ptr<SimState> state;

  struct replay_network_packet {
    std::uint64_t delivery_tick_count = 0;
    sim_packet packet;
  };
  std::mt19937_64 engine;
  std::vector<replay_network_packet> replay_packets;
  std::unique_ptr<NetworkedSimState> network_state;

  ISimState& istate() const {
    return network_state ? static_cast<ISimState&>(*network_state) : *state;
  }
};

ReplayViewer::~ReplayViewer() = default;

ReplayViewer::ReplayViewer(ui::GameStack& stack, data::ReplayReader&& replay,
                           const game_options_t& options)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender}
, impl_{std::make_unique<impl_t>(std::move(replay), options)} {
  auto conditions = impl_->reader.initial_conditions();
  if (options.replay_remote_players.empty()) {
    impl_->state = std::make_unique<SimState>(conditions);
    return;
  }

  NetworkedSimState::input_mapping mapping;
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    if (std::find(options.replay_remote_players.begin(), options.replay_remote_players.end(), i) ==
        options.replay_remote_players.end()) {
      mapping.local.player_numbers.emplace_back(i);
    } else {
      mapping.remote["remote"].player_numbers.emplace_back(i);
    }
  }
  impl_->network_state = std::make_unique<NetworkedSimState>(conditions, mapping);
}

void ReplayViewer::update_content(const ui::input_frame& input, ui::output_frame&) {
  set_sim_layer(impl_->istate(), stack(), *this);
  if (is_removed()) {
    return;
  }

  if (impl_->istate().game_over()) {
    stack().play_sound(sound::kMenuAccept);
    remove();
    return;
  }

  for (std::uint32_t i = 0; i < impl_->frame_count_multiplier; ++i) {
    if (impl_->state) {
      auto input = impl_->reader.next_tick_input_frames();
      impl_->state->update(input);
    } else {
      auto frames = impl_->reader.next_tick_input_frames();
      std::vector<input_frame> local_frames;
      impl_t::replay_network_packet packet;
      for (std::uint32_t i = 0; i < impl_->reader.initial_conditions().player_count; ++i) {
        if (std::find(impl_->options.replay_remote_players.begin(),
                      impl_->options.replay_remote_players.end(),
                      i) != impl_->options.replay_remote_players.end()) {
          packet.packet.input_frames.emplace_back(frames[i]);
        } else {
          local_frames.emplace_back(frames[i]);
        }
      }
      packet.packet.canonical_checksum = 0;
      auto current_tick = impl_->network_state->tick_count();
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
  impl_->render_state.set_dimensions(impl_->istate().dimensions());
  impl_->render_state.handle_output(impl_->istate(), handle_audio ? &stack().mixer() : nullptr,
                                    nullptr);
  impl_->render_state.update(nullptr);

  if (input.pressed(ui::key::kCancel)) {
    impl_->frame_count_multiplier *= 2;
  }
  if (input.pressed(ui::key::kAccept) && impl_->frame_count_multiplier > 1) {
    impl_->frame_count_multiplier /= 2;
  }

  if ((input.pressed(ui::key::kStart) || input.pressed(ui::key::kEscape))) {
    stack().add<PauseLayer>([this] { remove(); });
    return;
  }
}

void ReplayViewer::render_content(render::GlRenderer& r) const {
  const auto& render = impl_->istate().render(/* paused */ stack().top() != this);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r);  // TODO: can be merged with below?
  // TODO: instead of messing with trail alpha, render N frames ago first so trails aren't quite so
  // long?
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes,
                  /* trail alpha */ 1.f / (1.f + std::log2f(impl_->frame_count_multiplier)));
  render_hud(r, impl_->mode, render);

  auto progress =
      static_cast<float>(impl_->reader.current_input_frame()) / impl_->reader.total_input_frames();
  std::stringstream ss;
  ss << impl_->frame_count_multiplier << "X " << static_cast<std::uint32_t>(100 * progress) << "%";

  render_text(r,
              {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
               kDimensions.y / kTextSize.y - 3.f},
              ss.str(), kPanelTran);
}

}  // namespace ii
