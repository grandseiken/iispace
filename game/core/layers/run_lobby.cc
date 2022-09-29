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
#include "game/logic/sim/io/player.h"
#include "game/system/system.h"
#include <algorithm>

namespace ii {
namespace {
constexpr std::uint32_t kAllReadyTimerFrames = 240;

ustring mode_title(const initial_conditions& conditions) {
  switch (conditions.mode) {
  default:
  case game_mode::kNormal:
    return ustring::ascii("Normal mode");
  case game_mode::kBoss:
    return ustring::ascii("Boss mode");
  case game_mode::kHard:
    return ustring::ascii("Hard mode");
  case game_mode::kFast:
    return ustring::ascii("Fast mode");
  case game_mode::kWhat:
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

  auto broadcast = [&](const data::lobby_update_packet& packet) {
    auto bytes = data::write_lobby_update_packet(packet);
    if (!bytes) {
      stack().add<ErrorLayer>(ustring::ascii("Error sending lobby update: " + bytes.error()),
                              [this] { disconnect_and_remove(); });
      return;
    }
    System::send_message message;
    message.send_flags = System::send_flags::kSendReliable;
    message.channel = 0;
    message.bytes = *bytes;
    system.broadcast(message);
  };

  auto receive_broadcast = [&] {
    std::vector<System::received_message> messages;
    std::vector<data::lobby_update_packet> packets;
    system.receive(0, messages);
    for (const auto& m : messages) {
      if (!system.current_lobby() || !system.current_lobby()->host ||
          m.source_user_id != system.current_lobby()->host->id) {
        continue;
      }
      auto packet = data::read_lobby_update_packet(m.bytes);
      if (!packet) {
        stack().add<ErrorLayer>(ustring::ascii("Error receiving lobby update: " + packet.error()),
                                [this] { disconnect_and_remove(); });
        break;
      }
      packets.emplace_back(std::move(*packet));
    }
    return packets;
  };

  if (online_) {
    bool disconnected = !system.current_lobby() || (is_host && !conditions_) ||
        event_triggered(system, System::event_type::kLobbyDisconnected);
    if (disconnected) {
      stack().add<ErrorLayer>(ustring::ascii("Disconnected"), [this] { disconnect_and_remove(); });
      return;
    }

    if (is_host && event_triggered(system, System::event_type::kLobbyMemberEntered)) {
      data::lobby_update_packet packet;
      packet.conditions = conditions_;
      broadcast(packet);
    }
    if (!is_host) {
      for (const auto& packet : receive_broadcast()) {
        if (packet.conditions) {
          conditions_ = *packet.conditions;
        }
        if (packet.slots) {
          coordinator_->handle(*packet.slots);
        }
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
  if (conditions_ && coordinator_->game_ready()) {
    if (!all_ready_timer_) {
      all_ready_timer_ = kAllReadyTimerFrames;
    }
    if (*all_ready_timer_ % stack().fps() == 0) {
      output.sounds.emplace(sound::kPlayerShield);
    }
    if (!--*all_ready_timer_) {
      start_game();
    }
  } else {
    all_ready_timer_.reset();
  }
  if (all_ready_timer_) {
    auto s = "Game starting in " + std::to_string(*all_ready_timer_ / stack().fps());
    bottom_tabs_->set_active(1);
    all_ready_text_->set_text(ustring::ascii(s));
  } else {
    bottom_tabs_->set_active(0);
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

void RunLobbyLayer::start_game() {
  auto input_devices = coordinator_->input_devices();
  auto start_conditions = *conditions_;
  start_conditions.player_count = coordinator_->player_count();
  stack().add<SimLayer>(start_conditions, input_devices);
  clear_and_remove();
}

}  // namespace ii
