#include "game/core/layers/run_lobby.h"
#include "game/core/layers/common.h"
#include "game/core/layers/slot_coordinator.h"
#include "game/core/layers/utility.h"
#include "game/core/sim/sim_layer.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/data/packet.h"
#include "game/io/io.h"
#include "game/system/system.h"
#include <algorithm>
#include <utility>

namespace ii {
namespace {
constexpr std::uint32_t kStartTimerFrames = 240;
constexpr std::uint32_t kStartTimerLockFrames = 90;

ustring mode_title(const initial_conditions& conditions) {
  switch (conditions.mode) {
  default:
  case game_mode::kLegacy_Normal:
    return ustring::ascii("Normal mode");
  case game_mode::kLegacy_Boss:
    return ustring::ascii("Boss mode");
  case game_mode::kLegacy_Hard:
    return ustring::ascii("Hard mode");
  case game_mode::kLegacy_Fast:
    return ustring::ascii("Fast mode");
  case game_mode::kLegacy_What:
    return ustring::ascii("W-hat mode");
  }
  return {};
}
}  // namespace

RunLobbyLayer::~RunLobbyLayer() = default;

RunLobbyLayer::RunLobbyLayer(ui::GameStack& stack, std::optional<initial_conditions> conditions,
                             bool online)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer | ui::layer_flag::kNoAutoFocus}
, conditions_{conditions}
, online_{online} {
  set_bounds(rect{kUiDimensions});

  auto& panel = *add_back<ui::Panel>();
  panel.set_padding(kSpacing);
  auto& layout = *panel.add_back<ui::LinearLayout>();
  layout.set_spacing(kPadding.y).set_wrap_focus(true);

  auto& top = *layout.add_back<ui::Panel>();
  auto& subtop = *layout.add_back<ui::LinearLayout>();
  auto& main = *layout.add_back<ui::LinearLayout>();
  bottom_tabs_ = layout.add_back<ui::TabContainer>();
  auto& bottom = *bottom_tabs_->add_back<ui::LinearLayout>();
  all_ready_text_ = bottom_tabs_->add_back<ui::TextElement>();

  top.set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(kBackgroundColour);
  layout.set_absolute_size(top, kLargeFont.y + 2 * kPadding.y)
      .set_absolute_size(subtop, kLargeFont.y + 2 * kPadding.y)
      .set_absolute_size(*bottom_tabs_, kLargeFont.y + 2 * kPadding.y);
  bottom.set_spacing(kPadding.x).set_orientation(ui::orientation::kHorizontal);

  subtop.set_orientation(ui::orientation::kHorizontal);
  auto& subbox = *subtop.add_back<ui::Panel>();
  subbox.set_padding(kPadding);

  subtitle_ = subbox.add_back<ui::TextElement>();
  if (online_) {
    subtitle_->set_alignment(ui::alignment::kLeft);
    auto& invite_button = *subtop.add_back<ui::Button>();
    standard_button(invite_button)
        .set_alignment(ui::alignment::kCentered)
        .set_text(ustring::ascii("Invite"))
        .set_callback([this] { this->stack().system().show_invite_dialog(); });
    subtop.set_relative_weight(invite_button, 1.f / 4);
  } else {
    subtitle_->set_alignment(ui::alignment::kCentered);
  }
  subtitle_->set_font(render::font_id::kMonospace)
      .set_colour(kTextColour)
      .set_font_dimensions(kMediumFont)
      .set_drop_shadow(kDropShadow, .5f);

  title_ = top.add_back<ui::TextElement>();
  title_->set_font(render::font_id::kMonospaceBoldItalic)
      .set_colour(kHighlightColour)
      .set_font_dimensions(kLargeFont)
      .set_drop_shadow(kDropShadow, .5f)
      .set_alignment(ui::alignment::kCentered);

  all_ready_text_->set_font(render::font_id::kMonospaceBoldItalic)
      .set_colour(kHighlightColour)
      .set_font_dimensions(kLargeFont)
      .set_drop_shadow(kDropShadow, .5f)
      .set_alignment(ui::alignment::kCentered);

  back_button_ = bottom.add_back<ui::Button>();
  const auto* back_text = online_ ? "Leave lobby" : "Back";
  standard_button(*back_button_).set_text(ustring::ascii(back_text)).set_callback([this] {
    disconnect_and_remove();
  });

  main.set_orientation(ui::orientation::kHorizontal);
  main.set_spacing(kPadding.x);
  coordinator_ = std::make_unique<LobbySlotCoordinator>(stack, main, online_);
}

void RunLobbyLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  GameLayer::update_content(input, output);
  ustring title_text;
  if (conditions_) {
    title_text = mode_title(*conditions_);
  }

  auto& system = stack().system();
  bool is_host = online_ && system.current_lobby() && !system.current_lobby()->host;

  auto send_update = [&](std::optional<std::uint64_t> user_id,
                         const data::lobby_update_packet& packet) {
    auto bytes = data::write_lobby_update_packet(packet);
    if (!bytes) {
      stack().add<ErrorLayer>(ustring::ascii("Error sending lobby update: " + bytes.error()),
                              [this] { disconnect_and_remove(); });
      return;
    }
    System::send_message message;
    message.send_flags = System::send_flags::kSendReliable;
    message.channel = data::lobby_update_packet::kChannel;
    message.bytes = *bytes;
    if (user_id) {
      system.send_to(*user_id, message);
    } else {
      system.broadcast(message);
    }
  };

  auto send_request = [&](const data::lobby_request_packet& packet) {
    auto bytes = data::write_lobby_request_packet(packet);
    if (!bytes) {
      stack().add<ErrorLayer>(ustring::ascii("Error sending lobby request: " + bytes.error()),
                              [this] { disconnect_and_remove(); });
      return;
    }
    System::send_message message;
    message.send_flags = System::send_flags::kSendReliable;
    message.channel = data::lobby_request_packet::kChannel;
    message.bytes = *bytes;
    system.send_to_host(message);
  };

  auto receive_update = [&] {
    std::vector<System::received_message> messages;
    system.receive(data::lobby_update_packet::kChannel, messages);
    std::vector<data::lobby_update_packet> packets;
    for (const auto& m : messages) {
      if (!system.current_lobby() || !system.current_lobby()->host ||
          m.source_user_id != system.current_lobby()->host->id) {
        continue;
      }
      auto packet = data::read_lobby_update_packet(m.bytes);
      if (!packet) {
        stack().add<ErrorLayer>(ustring::ascii("Error reading lobby update: " + packet.error()),
                                [this] { disconnect_and_remove(); });
        break;
      }
      packets.emplace_back(std::move(*packet));
    }
    return packets;
  };

  auto receive_requests = [&] {
    std::vector<System::received_message> messages;
    system.receive(data::lobby_request_packet::kChannel, messages);
    std::vector<std::pair<std::uint64_t, data::lobby_request_packet>> packets;
    for (const auto& m : messages) {
      if (!system.current_lobby() || system.current_lobby()->host) {
        continue;
      }
      auto packet = data::read_lobby_request_packet(m.bytes);
      if (!packet) {
        stack().add<ErrorLayer>(ustring::ascii("Error reading lobby request: " + packet.error()),
                                [this] { disconnect_and_remove(); });
        break;
      }
      packets.emplace_back(m.source_user_id, std::move(*packet));
    }
    return packets;
  };

  auto setup_network_mapping = [&](const data::lobby_update_packet::start_game_setup& setup) {
    network_input_mapping m;
    for (std::uint32_t i = 0; i < setup.players.size(); ++i) {
      const auto& p = setup.players[i];
      if (p.user_id == stack().system().local_user().id) {
        m.local.player_numbers.emplace_back(i);
      } else {
        m.remote[std::to_string(p.user_id)].player_numbers.emplace_back(i);
      }
    }
    return m;
  };

  if (online_) {
    bool disconnected = !system.current_lobby() || (is_host && !conditions_) ||
        event_triggered(system, System::event_type::kLobbyDisconnected);
    if (disconnected) {
      stack().add<ErrorLayer>(ustring::ascii("Disconnected"), [this] { disconnect_and_remove(); });
      return;
    }

    std::optional<std::uint64_t> host_id;
    if (system.current_lobby()->host) {
      host_id = system.current_lobby()->host->id;
    }
    if (host_id != host_) {
      host_ = host_id;
      coordinator_->set_dirty();
    }
    if (event_triggered(stack().system(), System::event_type::kMessagingSessionFailed)) {
      coordinator_->set_dirty();
    }

    for (const auto& packet : receive_update()) {
      if (is_host) {
        break;
      }
      if (packet.conditions) {
        conditions_ = *packet.conditions;
      }
      if (packet.slots) {
        coordinator_->handle(*packet.slots, output);
      }
      if (packet.start & data::lobby_update_packet::kStartTimer) {
        start_timer_ = kStartTimerFrames;
      } else if (packet.start & data::lobby_update_packet::kCancelTimer) {
        start_timer_.reset();
        coordinator_->unlock();
      } else if (packet.start & data::lobby_update_packet::kLockSlots) {
        coordinator_->lock();
      } else if (packet.setup && (packet.start & data::lobby_update_packet::kStartGame)) {
        start_game(setup_network_mapping(*packet.setup));
      }
    }
    for (const auto& pair : receive_requests()) {
      if (is_host) {
        coordinator_->handle(pair.first, pair.second, output);
      }
    }

    title_text += is_host
        ? ustring::ascii(" (Host)")
        : ustring::ascii(" (") + system.current_lobby()->host->name + ustring::ascii("'s game)");
    ustring s;
    for (const auto& m : system.current_lobby()->members) {
      if (!s.empty()) {
        s += ustring::ascii(", ");
      }
      s += m.name;
    }
    subtitle_->set_text(ustring::ascii("In lobby: ") + s);
  } else {
    subtitle_->set_text(ustring::ascii("Local game"));
  }
  title_->set_text(std::move(title_text));

  if (coordinator_->update(input.join_game_inputs)) {
    output.sounds.emplace(sound::kMenuAccept);
    back_button_->unfocus();
  }

  bool all_ready = (!online_ || is_host) && conditions_ && coordinator_->game_ready() &&
      !event_triggered(system, System::event_type::kLobbyMemberEntered) &&
      !event_triggered(system, System::event_type::kLobbyMemberLeft) &&
      !event_triggered(system, System::event_type::kMessagingSessionFailed);

  auto schedule = [&](schedule_t& s) -> std::uint32_t {
    s.clear();
    if (!system.current_lobby()) {
      return 0;
    }
    std::uint32_t frames_max = 0;
    for (const auto& m : system.current_lobby()->members) {
      if (m.id == system.local_user().id) {
        continue;
      }
      auto latency_seconds = static_cast<float>(system.session(m.id)->ping_ms) / 2000.f;
      auto latency_frames = static_cast<std::uint32_t>(std::ceil(stack().fps() * latency_seconds));
      latency_frames = std::min(latency_frames, stack().fps());
      frames_max = std::max(frames_max, latency_frames);
      s[m.id] = latency_frames;
    }
    return frames_max;
  };

  auto process = [&](schedule_t& s, std::uint32_t timer, auto&& make_packet) {
    for (auto it = s.begin(); it != s.end();) {
      if (timer > it->second) {
        ++it;
        continue;
      }
      send_update(it->first, make_packet());
      it = s.erase(it);
    }
    return !timer;
  };

  auto make_start_timer_packet = [] {
    data::lobby_update_packet packet;
    packet.start = data::lobby_update_packet::kStartTimer;
    return packet;
  };

  auto make_start_game_packet = [&] {
    data::lobby_update_packet packet;
    packet.start = data::lobby_update_packet::kStartGame;
    packet.conditions = conditions_;
    packet.setup = coordinator_->start_game_setup();
    return packet;
  };

  std::uint32_t broadcast_start_flags = data::lobby_update_packet::kNone;
  if (!online_) {
    if (!all_ready) {
      start_timer_.reset();
    } else if (!start_timer_) {
      start_timer_ = kStartTimerFrames;
    }
  } else if (is_host &&
             (!system.current_lobby() || (start_timer_ && !host_countdown_) ||
              (host_countdown_ && !all_ready))) {
    broadcast_start_flags |= data::lobby_update_packet::kCancelTimer;
    start_timer_.reset();
    host_countdown_.reset();
  } else if (is_host && all_ready && !host_countdown_) {
    auto& hc = host_countdown_.emplace();
    hc.countdown_schedule.emplace();
    hc.timer = schedule(*hc.countdown_schedule);
  } else if (!is_host) {
    host_countdown_.reset();
  }

  if (host_countdown_) {
    auto& hc = *host_countdown_;
    hc.timer && --hc.timer;
    if (hc.countdown_schedule &&
        process(*hc.countdown_schedule, hc.timer, make_start_timer_packet)) {
      start_timer_ = kStartTimerFrames;
      hc.countdown_schedule.reset();
    }
    if (hc.start_game_schedule &&
        process(*hc.start_game_schedule, hc.timer, make_start_game_packet)) {
      start_game(setup_network_mapping(coordinator_->start_game_setup()));
    }
  }

  if (start_timer_) {
    if (*start_timer_ && *start_timer_ % stack().fps() == 0) {
      output.sounds.emplace(sound::kPlayerShield);
    }
    *start_timer_ && --*start_timer_;
    if (host_countdown_ &&
        *start_timer_ == kStartTimerLockFrames + host_countdown_->max_frame_latency) {
      broadcast_start_flags = data::lobby_update_packet::kLockSlots;
    }
    if (!*start_timer_ && !online_) {
      start_game(/* local */ std::nullopt);
    } else if (!*start_timer_ && host_countdown_) {
      auto& hc = *host_countdown_;
      if (!hc.start_game_schedule) {
        hc.start_game_schedule.emplace();
        hc.timer = schedule(*hc.start_game_schedule);
      }
    }
  }

  if (start_timer_) {
    auto s = "Game starting in " + std::to_string(*start_timer_ / stack().fps());
    bottom_tabs_->set_active(1);
    all_ready_text_->set_text(ustring::ascii(s));
  } else {
    bottom_tabs_->set_active(0);
  }

  if (online_) {
    data::lobby_update_packet packet;
    bool force_refresh =
        is_host && event_triggered(system, System::event_type::kLobbyMemberEntered);
    if (force_refresh) {
      packet.conditions = conditions_;
      coordinator_->set_dirty();
    }
    packet.slots = coordinator_->host_slot_update();
    packet.start = broadcast_start_flags;
    if (packet.conditions || packet.slots || packet.start) {
      send_update(/* broadcast */ std::nullopt, packet);
    }

    if (auto request = coordinator_->client_request(); request) {
      send_request(*request);
    }
  }

  if (auto index = stack().input().assignment(ui::input_device_id::kbm()); index) {
    stack().set_cursor_hue(player_colour(*index).x);
  } else {
    stack().clear_cursor_hue();
  }

  if (input.pressed(ui::key::kCancel)) {
    output.sounds.emplace(sound::kMenuAccept);
    disconnect_and_remove();
  }
}

void RunLobbyLayer::disconnect_and_remove() {
  if (online_) {
    stack().system().leave_lobby();
  }
  clear_and_remove();
}

void RunLobbyLayer::clear_and_remove() {
  stack().input().clear_assignments();
  stack().clear_cursor_hue();
  remove();
}

void RunLobbyLayer::start_game(std::optional<network_input_mapping> network) {
  auto input_devices = coordinator_->input_devices();
  auto start_conditions = *conditions_;
  start_conditions.player_count = coordinator_->player_count();
  stack().add<SimLayer>(start_conditions, input_devices, std::move(network));
  clear_and_remove();
}

}  // namespace ii
