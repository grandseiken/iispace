#include "game/core/sim/replay_viewer.h"
#include "game/core/game_options.h"
#include "game/core/sim/hud_layer.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/data/replay.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <random>

namespace ii {
namespace {
constexpr std::array kSpeedFrames = {1u, 2u, 4u, 8u, 16u, 32u, 64u};
constexpr std::array kSpeedPreRenderFrames = {0u, 1u, 3u, 6u, 14u, 30u, 61u};
}  // namespace

struct ReplayViewer::impl_t {
  impl_t(data::ReplayReader&& reader)
  : reader{std::move(reader)}
  , mode{this->reader.initial_conditions().mode}
  , render_state{this->reader.initial_conditions().seed}
  , engine{std::random_device{}()} {}

  HudLayer* hud = nullptr;
  data::ReplayReader reader;
  game_mode mode;
  RenderState render_state;
  transient_render_state transients;
  std::uint32_t speed = 0;
  std::uint32_t audio_tick = 0;
  std::unique_ptr<SimState> state;

  struct replay_network_packet {
    std::uint64_t delivery_tick_count = 0;
    data::sim_packet packet;
  };
  std::mt19937_64 engine;
  std::vector<replay_network_packet> replay_packets;
  std::unique_ptr<NetworkedSimState> network_state;

  ISimState& istate() const {
    return network_state ? static_cast<ISimState&>(*network_state) : *state;
  }
};

ReplayViewer::~ReplayViewer() = default;

ReplayViewer::ReplayViewer(ui::GameStack& stack, data::ReplayReader&& replay)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer}
, impl_{std::make_unique<impl_t>(std::move(replay))} {
  auto conditions = impl_->reader.initial_conditions();
  const auto& remote_players = stack.options().replay_remote_players;
  if (remote_players.empty()) {
    impl_->state = std::make_unique<SimState>(conditions);
    return;
  }

  network_input_mapping mapping;
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    if (std::find(remote_players.begin(), remote_players.end(), i) == remote_players.end()) {
      mapping.local.player_numbers.emplace_back(i);
    } else {
      mapping.remote["remote"].player_numbers.emplace_back(i);
    }
  }
  impl_->network_state = std::make_unique<NetworkedSimState>(conditions, mapping);
}

void ReplayViewer::update_content(const ui::input_frame& input, ui::output_frame&) {
  set_bounds(rect{impl_->istate().dimensions()});
  stack().set_fps(impl_->istate().fps());
  if (!impl_->hud) {
    impl_->hud = stack().add<HudLayer>(impl_->mode);
  }
  auto progress =
      static_cast<float>(impl_->reader.current_input_frame()) / impl_->reader.total_input_frames();
  impl_->hud->set_status_text(std::to_string(kSpeedFrames[impl_->speed]) + "X " +
                              std::to_string(static_cast<std::uint32_t>(100 * progress)) + "%");

  if (is_removed()) {
    return;
  }

  if (impl_->istate().game_over()) {
    stack().play_sound(sound::kMenuAccept);
    impl_->hud->remove();
    remove();
    return;
  }

  const auto& remote_players = stack().options().replay_remote_players;
  for (std::uint32_t i = 0; i < kSpeedFrames[impl_->speed]; ++i) {
    if (impl_->state) {
      auto input = impl_->reader.next_tick_input_frames();
      impl_->state->update(input);
    } else {
      auto frames = impl_->reader.next_tick_input_frames();
      std::vector<input_frame> local_frames;
      impl_t::replay_network_packet packet;
      for (std::uint32_t i = 0; i < impl_->reader.initial_conditions().player_count; ++i) {
        if (std::find(remote_players.begin(), remote_players.end(), i) != remote_players.end()) {
          packet.packet.input_frames.emplace_back(frames[i]);
        } else {
          local_frames.emplace_back(frames[i]);
        }
      }
      packet.packet.canonical_checksum = 0;
      auto current_tick = impl_->network_state->tick_count();
      packet.packet.canonical_tick_count = packet.packet.tick_count = current_tick;

      std::uniform_int_distribution<std::uint64_t> d{
          stack().options().replay_min_tick_delivery_delay,
          stack().options().replay_max_tick_delivery_delay};
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
    if (1 + i == kSpeedPreRenderFrames[impl_->speed]) {
      impl_->istate().render(impl_->transients, /* paused */ false);
    }
  }

  bool handle_audio = !(impl_->audio_tick++ % (4 * (1 + impl_->speed / 2)));
  impl_->render_state.set_dimensions(impl_->istate().dimensions());
  impl_->render_state.handle_output(impl_->istate(), handle_audio ? &stack().mixer() : nullptr,
                                    nullptr);
  impl_->render_state.update(nullptr);

  if (input.pressed(ui::key::kUp)) {
    impl_->speed = std::min(6u, impl_->speed + 1);
  }
  if (input.pressed(ui::key::kDown) && impl_->speed) {
    --impl_->speed;
  }

  if (input.pressed(ui::key::kStart) || input.pressed(ui::key::kEscape) ||
      sim_should_pause(stack())) {
    stack().add<PauseLayer>([this] {
      impl_->hud->remove();
      remove();
    });
    return;
  }
}

void ReplayViewer::render_content(render::GlRenderer& r) const {
  auto style =
      is_legacy_mode(impl_->mode) ? render::shape_style::kNone : render::shape_style::kStandard;
  const auto& render =
      impl_->istate().render(impl_->transients, /* paused */ stack().top() != impl_->hud);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r, style);  // TODO: can be merged with below?
  impl_->hud->set_data(render);
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes, style, /* trail alpha */ 1.f);
}

}  // namespace ii
