#include "game/core/sim/sim_layer.h"
#include "game/core/game_options.h"
#include "game/core/layers/utility.h"
#include "game/core/sim/hud_layer.h"
#include "game/core/sim/input_adapter.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/core/ui/input.h"
#include "game/data/packet.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include "game/system/system.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <sstream>
#include <vector>

namespace ii {

// TODO: handle input device issues (not enough, unplugged, etc).
struct SimLayer::impl_t {
  impl_t(io::IoLayer& io_layer, const initial_conditions& conditions,
         std::span<const ui::input_device_id> input_devices,
         std::optional<network_input_mapping> network, const game_options_t& options)
  : options{options}
  , mode{conditions.mode}
  , render_state{conditions.seed}
  , input{io_layer, input_devices}
  , writer{conditions}
  , network{std::move(network)} {
    if (this->network) {
      networked_state = std::make_unique<NetworkedSimState>(conditions, *this->network, &writer);
    } else {
      state = std::make_unique<SimState>(conditions, &writer, options.ai_players);
    }
  }

  ISimState& istate() const {
    return networked_state ? static_cast<ISimState&>(*networked_state) : *state;
  }

  HudLayer* hud = nullptr;
  std::uint32_t audio_tick = 0;
  game_options_t options;
  game_mode mode;
  RenderState render_state;
  SimInputAdapter input;
  data::ReplayWriter writer;
  std::unique_ptr<SimState> state;
  std::unique_ptr<NetworkedSimState> networked_state;
  std::optional<network_input_mapping> network;
};

SimLayer::~SimLayer() = default;

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   std::span<const ui::input_device_id> input_devices,
                   std::optional<network_input_mapping> network)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer | ui::layer_flag::kCaptureCursor}
, impl_{std::make_unique<impl_t>(stack.io_layer(), conditions, input_devices, std::move(network),
                                 stack.options())} {}

void SimLayer::update_content(const ui::input_frame& ui_input, ui::output_frame&) {
  set_bounds(rect{impl_->istate().dimensions()});
  stack().set_fps(impl_->istate().fps());
  if (!impl_->hud) {
    impl_->hud = stack().add<HudLayer>(impl_->mode);
  }
  if (is_removed()) {
    return;
  }
  if (impl_->istate().game_over()) {
    end_game();
    stack().play_sound(sound::kMenuAccept);
    impl_->hud->remove();
    remove();
    return;
  }

  impl_->input.set_game_dimensions(impl_->istate().dimensions());
  auto sim_input = impl_->input.get();
  if (impl_->networked_state) {
    networked_update(std::move(sim_input));
  } else {
    impl_->state->ai_think(sim_input);
    impl_->state->update(sim_input);
  }

  bool handle_audio = !(impl_->audio_tick++ % 4);
  impl_->render_state.set_dimensions(impl_->istate().dimensions());
  impl_->render_state.handle_output(impl_->istate(), handle_audio ? &stack().mixer() : nullptr,
                                    &impl_->input);
  impl_->render_state.update(&impl_->input);

  if (ui_input.pressed(ui::key::kStart) || ui_input.pressed(ui::key::kEscape) ||
      sim_should_pause(stack())) {
    stack().add<PauseLayer>([this] {
      end_game();
      impl_->hud->remove();
      remove();
    });
    return;
  }
}

void SimLayer::render_content(render::GlRenderer& r) const {
  const auto& render = impl_->istate().render(/* paused */ stack().top() != impl_->hud);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r);  // TODO: can be merged with below?
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes, /* trail alpha */ 1.f);
  impl_->hud->set_data(render);
}

void SimLayer::networked_update(std::vector<input_frame>&& local_input) {
  // TODO: timing.
  auto disconnect_with_error = [this](ustring message) {
    stack().add<ErrorLayer>(std::move(message), [this] {
      impl_->hud->remove();
      remove();
    });
  };

  bool disconnected = !stack().system().current_lobby() ||
      event_triggered(stack().system(), System::event_type::kLobbyDisconnected) ||
      event_triggered(stack().system(), System::event_type::kMessagingSessionFailed);
  if (disconnected) {
    disconnect_with_error(ustring::ascii("Disconnected"));
    return;
  }

  std::vector<System::received_message> messages;
  stack().system().receive(data::sim_packet::kChannel, messages);
  for (const auto& m : messages) {
    auto packet = data::read_sim_packet(m.bytes);
    if (!packet) {
      disconnect_with_error(ustring::ascii("Error reading sim packet: " + packet.error()));
      return;
    }
    impl_->networked_state->input_packet(std::to_string(m.source_user_id), *packet);
  }

  std::uint32_t frame_count = 1;
  if (stack().system().current_lobby()->host) {
    // Try to sync timing with host.
    static constexpr std::uint32_t kFrameSyncWindowSize = 10u;
    auto host_id = stack().system().current_lobby()->host->id;
    auto host_stats = impl_->networked_state->remote(std::to_string(host_id));
    auto host_session = stack().system().session(host_id);
    if (host_session) {
      auto estimated_host_tick = host_stats.latest_tick +
          static_cast<std::uint32_t>(std::ceil(stack().fps() *
                                               static_cast<float>(host_session->ping_ms) / 2000.f));
      auto local_tick = impl_->networked_state->tick_count() + 1;
      if (local_tick >= estimated_host_tick + kFrameSyncWindowSize) {
        frame_count = 0;
      } else if (local_tick + kFrameSyncWindowSize <= estimated_host_tick) {
        frame_count = 2;
      }
    }
  }

  for (std::uint32_t i = 0; i < frame_count; ++i) {
    auto packets = impl_->networked_state->update(local_input);
    for (const auto& packet : packets) {
      auto bytes = data::write_sim_packet(packet);
      if (!bytes) {
        disconnect_with_error(ustring::ascii("Error sending sim packet: " + bytes.error()));
        return;
      }
      System::send_message message;
      message.bytes = *bytes;
      message.channel = data::sim_packet::kChannel;
      message.send_flags = System::send_flags::kSendReliable;
      if (i + 1 == frame_count) {
        message.send_flags |= System::send_flags::kSendNoNagle;
      }
      stack().system().broadcast(message);
    }
  }

  for (std::uint32_t i = 0; i < kMaxPlayers; ++i) {
    for (const auto& pair : impl_->network->remote) {
      const auto& numbers = pair.second.player_numbers;
      if (!std::count(numbers.begin(), numbers.end(), i)) {
        continue;
      }
      std::uint64_t id = 0;
      auto result = std::from_chars(pair.first.data(), pair.first.data() + pair.first.size(), id);
      if (result.ec != std::errc{} || result.ptr != pair.first.data() + pair.first.size()) {
        continue;
      }
      auto stats = impl_->networked_state->remote(pair.first);
      auto session = stack().system().session(id);
      if (!session) {
        continue;
      }

      std::string debug;
      debug += "ping: " + std::to_string(session->ping_ms);
      debug += "\nquality: " + std::to_string(session->quality);

      auto estimated_tick = stats.latest_tick +
          static_cast<std::uint32_t>(std::ceil(stack().fps() *
                                               static_cast<float>(session->ping_ms) / 2000.f));

      auto fdiff = static_cast<std::int64_t>(estimated_tick) -
          static_cast<std::int64_t>(impl_->networked_state->tick_count());
      debug += "\nfdiff: " + std::to_string(fdiff);
      debug += "\nnpred: " + std::to_string(stats.latest_tick - stats.canonical_tick);
      impl_->hud->set_debug_text(i, std::move(debug));
    }
  }

  if (!impl_->networked_state->checksum_failed_remote_ids().empty()) {
    disconnect_with_error(ustring::ascii("Desync"));
  }
}

void SimLayer::end_game() {
  auto results = impl_->istate().results();
  if (impl_->mode == game_mode::kNormal || impl_->mode == game_mode::kBoss) {
    stack().savegame().bosses_killed |= results.bosses_killed();
  } else {
    stack().savegame().hard_mode_bosses_killed |= results.bosses_killed();
  }
  stack().write_savegame();
  stack().write_replay(impl_->writer, "untitled", results.score);
}

}  // namespace ii
