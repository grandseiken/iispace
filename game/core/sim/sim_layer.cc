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
namespace {

std::uint64_t estimate_tick(std::uint32_t fps, const NetworkedSimState::remote_stats& stats,
                            const System::session_info& session) {
  return stats.latest_tick +
      static_cast<std::uint64_t>(
             std::ceil(static_cast<float>(fps) * static_cast<float>(session.ping_ms) / 2000.f));
};

}  // namespace

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
  transient_render_state transients;
  SimInputAdapter input;
  data::ReplayWriter writer;
  std::unique_ptr<SimState> state;
  std::unique_ptr<NetworkedSimState> networked_state;
  std::optional<network_input_mapping> network;

  struct network_frame_diff_t {
    bool ahead = false;
    std::uint32_t frame_count = 0;
  };
  std::optional<network_frame_diff_t> network_frame_diff;

  std::uint32_t networked_frame_advance_count(std::uint32_t local_tick, std::uint32_t host_tick) {
    static constexpr std::uint32_t kFrameSyncWindowSize = 8u;
    static constexpr std::uint32_t kFrameSyncInWindowFrames = 12u;
    if (local_tick >= host_tick + kFrameSyncWindowSize) {
      network_frame_diff.reset();
      return 0u;
    }
    if (local_tick + kFrameSyncWindowSize <= host_tick) {
      network_frame_diff.reset();
      return 2u;
    }
    if (local_tick <= host_tick + 1u && host_tick <= local_tick + 1u) {
      network_frame_diff.reset();
      return 1u;
    }
    if (local_tick > host_tick) {
      if (!network_frame_diff || network_frame_diff->ahead) {
        network_frame_diff = network_frame_diff_t{};
      }
      if (++network_frame_diff->frame_count == kFrameSyncInWindowFrames) {
        network_frame_diff.reset();
        return 0u;
      }
      return 1u;
    }
    if (!network_frame_diff || !network_frame_diff->ahead) {
      network_frame_diff = network_frame_diff_t{};
      network_frame_diff->ahead = true;
    }
    if (++network_frame_diff->frame_count == kFrameSyncInWindowFrames) {
      network_frame_diff.reset();
      return 2u;
    }
    return 1u;
  }
};

SimLayer::~SimLayer() = default;

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   std::span<const ui::input_device_id> input_devices,
                   std::optional<network_input_mapping> network)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer | ui::layer_flag::kCaptureCursor}
, impl_{std::make_unique<impl_t>(stack.io_layer(), conditions, input_devices, std::move(network),
                                 stack.options())} {}

void SimLayer::update_content(const ui::input_frame& ui_input, ui::output_frame&) {
  set_bounds(irect{impl_->istate().dimensions()});
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

  bool handle_audio = !(impl_->audio_tick++ % (stack().fps() >= 60 ? 5 : 4));
  impl_->render_state.set_dimensions(impl_->istate().dimensions());
  std::vector<render::background::update> background_updates;
  impl_->render_state.handle_output(impl_->istate(), background_updates,
                                    handle_audio ? &stack().mixer() : nullptr, &impl_->input);
  impl_->render_state.update(&impl_->input);
  for (const auto& update : background_updates) {
    stack().update_background(update);
  }

  // TODO: handle pausing in multiplayer.
  // TODO: spacebar shouldn't pause on keyboard; but start should still pause on gamepad.
  bool should_pause = (ui_input.pressed(ui::key::kStart) || ui_input.pressed(ui::key::kEscape) ||
                       sim_should_pause(stack()));
  if (!impl_->network && should_pause) {
    stack().add<PauseLayer>([this] {
      end_game();
      impl_->hud->remove();
      remove();
    });
    return;
  }
}

void SimLayer::render_content(render::GlRenderer& r) const {
  auto style =
      is_legacy_mode(impl_->mode) ? render::shape_style::kNone : render::shape_style::kStandard;
  auto& render =
      impl_->istate().render(impl_->transients, /* paused */ stack().top() != impl_->hud);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(render.shapes, render.fx);
  for (const auto& panel : render.panels) {
    r.render_panel(panel);
  }
  r.clear_depth();
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes, render.fx, style);
  impl_->hud->set_data(render);
}

std::string SimLayer::network_debug_text(std::uint32_t index) {
  const auto& local_players = impl_->network->local.player_numbers;
  if (std::count(local_players.begin(), local_players.end(), index)) {
    return "fpred: " +
        std::to_string(impl_->networked_state->predicted().tick_count() -
                       impl_->networked_state->canonical().tick_count());
  }

  for (const auto& pair : impl_->network->remote) {
    const auto& players = pair.second.player_numbers;
    if (!std::count(players.begin(), players.end(), index)) {
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
    auto fdiff = static_cast<std::int64_t>(estimate_tick(stack().fps(), stats, *session)) -
        static_cast<std::int64_t>(impl_->networked_state->tick_count());

    std::string debug;
    debug += "ping: " + std::to_string(session->ping_ms);
    debug += "\nquality: " + std::to_string(session->quality);
    debug += "\nfdiff: " + std::to_string(fdiff);
    debug += "\nfpred: " + std::to_string(stats.latest_tick - stats.canonical_tick);
    return debug;
  }
  return {};
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
    // Sync timing with host.
    auto host_id = stack().system().current_lobby()->host->id;
    auto host_stats = impl_->networked_state->remote(std::to_string(host_id));
    auto host_session = stack().system().session(host_id);
    if (host_session) {
      auto estimated_host_tick = estimate_tick(stack().fps(), host_stats, *host_session);
      auto local_tick = impl_->networked_state->tick_count() + 1;
      frame_count = impl_->networked_frame_advance_count(local_tick, estimated_host_tick);
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
    impl_->hud->set_debug_text(i, network_debug_text(i));
  }

  if (!impl_->networked_state->checksum_failed_remote_ids().empty()) {
    disconnect_with_error(ustring::ascii("Desync"));
  }
}

void SimLayer::end_game() {
  auto results = impl_->istate().results();
  if (impl_->mode == game_mode::kLegacy_Normal || impl_->mode == game_mode::kLegacy_Boss) {
    stack().savegame().bosses_killed |= results.bosses_killed();
  } else if (impl_->mode == game_mode::kLegacy_Hard || impl_->mode == game_mode::kLegacy_Fast ||
             impl_->mode == game_mode::kLegacy_What) {
    stack().savegame().hard_mode_bosses_killed |= results.bosses_killed();
  }
  stack().write_savegame();
  stack().write_replay(impl_->writer, "untitled", results.score);
}

}  // namespace ii
