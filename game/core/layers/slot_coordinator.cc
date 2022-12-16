#include "game/core/layers/slot_coordinator.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"
#include "game/system/system.h"
#include <algorithm>

namespace ii {

// TODO: assignment panels currently fixed colour/number, so we with dropouts we can end up with
// e.g. players 0, 1 and 3 going into the game and then inconsistent player colours.
// TODO: indication that in single-player mode any combination of controller/KBM can be used.
class LobbySlotPanel : public ui::Panel {
public:
  LobbySlotPanel(Element* parent, std::size_t index)
  : ui::Panel{parent}, index_{index}, main_{add_back<ui::TabContainer>()} {
    waiting_text_ = main_->add_back<ui::TextElement>();
    auto& active = *main_->add_back<ui::Panel>();
    waiting_text_->set_font({render::font_id::kMonospaceItalic, kSemiLargeFont})
        .set_colour(kTextColour)
        .set_drop_shadow(kDropShadow, .5f)
        .set_alignment(render::alignment::kCentered)
        .set_multiline(true);

    auto colour = colour::kWhite0;
    auto focus_colour = colour;
    focus_colour.z += .125f;

    auto& layout = *active.add_back<ui::LinearLayout>();
    auto& l_top = *layout.add_back<ui::LinearLayout>();

    config_tab_ = layout.add_back<ui::TabContainer>();
    auto& l_bottom = *config_tab_->add_back<ui::LinearLayout>();
    l_bottom.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);
    controller_text_ = l_top.add_back<ui::TextElement>();
    controller_text_->set_alignment(render::alignment::kCentered)
        .set_colour(colour)
        .set_font({render::font_id::kMonospaceItalic, kSemiLargeFont})
        .set_drop_shadow(kDropShadow, .5f)
        .set_multiline(true);
    ready_button_ = &standard_button(*l_bottom.add_back<ui::Button>())
                         .set_text(ustring::ascii("Ready"))
                         .set_text_colour(colour, focus_colour)
                         .set_callback([this] { ready(); });
    leave_button_ = &standard_button(*l_bottom.add_back<ui::Button>())
                         .set_text(ustring::ascii("Leave"))
                         .set_text_colour(colour, focus_colour)
                         .set_callback([this] {
                           if (!locked_) {
                             clear();
                           }
                         });
    l_bottom.set_absolute_size(*ready_button_, kLargeFont.y + 2 * kPadding.y);
    l_bottom.set_absolute_size(*leave_button_, kLargeFont.y + 2 * kPadding.y);

    config_tab_->add_back<ui::Panel>();
    ready_text_ = config_tab_->add_back<ui::TextElement>();
    ready_text_->set_text(ustring::ascii("Ready"))
        .set_alignment(render::alignment::kCentered)
        .set_colour(colour)
        .set_font({render::font_id::kMonospaceBoldItalic, kLargeFont})
        .set_drop_shadow(kDropShadow, .5f);
  }

  void set_colour(const glm::vec4& colour) {
    auto focus_colour = colour;
    focus_colour.z += .125f;
    ready_text_->set_colour(colour);
    controller_text_->set_colour(colour);
    ready_button_->set_text_colour(colour, focus_colour);
    leave_button_->set_text_colour(colour, focus_colour);
  }

  void set_joining(bool joining) { joining_ = joining; }
  void set_locked(bool locked) { locked_ = locked; }

  void set_username(ustring s) {
    owner_username_ = std::move(s);
    refresh_controller_text();
  }

  void set_remote(bool is_ready) {
    assigned_id_.reset();
    main_->set_active(1u);
    config_tab_->set_active(is_ready ? 2u : 1u);
    controller_name_.reset();
    refresh_controller_text();
  }

  void assign(ui::input_device_id id, std::optional<std::string> controller_name) {
    assigned_id_ = id;
    assign_input_root(index_);
    main_->set_active(1u);
    config_tab_->set_active(0u);
    if (id.controller_index) {
      controller_name_ = std::move(controller_name);
      main_->focus();
    } else {
      controller_name_.reset();
    }
    refresh_controller_text();
  }

  void ready() { config_tab_->set_active(2u); }
  void clear() {
    if (auto* e = main_->focused_element()) {
      e->unfocus();
    }
    controller_name_.reset();
    assigned_id_.reset();
    config_tab_->set_active(0);
    main_->set_active(0);
    clear_input_root();
  }

  bool is_assigned() const { return assigned_id_.has_value(); }
  bool is_ready() const { return config_tab_->active_index() == 2u; }
  ui::input_device_id assigned_input_device() const { return *assigned_id_; }

protected:
  void refresh_controller_text() {
    ustring s;
    if (owner_username_) {
      s = *owner_username_;
    } else {
      s = ustring::ascii("Player " + std::to_string(index_ + 1));
    }
    if (!assigned_id_) {
    } else if (assigned_id_->controller_index) {
      s += ustring::ascii(std::string{"\n"} + controller_name_.value_or("Controller"));
    } else {
      s += ustring::ascii("\nMouse and keyboard");
    }
    controller_text_->set_text(std::move(s));
  }

  void update_content(const ui::input_frame& input, ui::output_frame& output) override {
    ui::Panel::update_content(input, output);

    if (!locked_ && input.pressed(ui::key::kCancel) && main_->active_index()) {
      if (config_tab_->active_index()) {
        config_tab_->set_active(0);
      } else {
        clear();
      }
      output.sounds.emplace(sound::kMenuAccept);
    }

    if (joining_) {
      waiting_text_->set_text(ustring::ascii("Joining..."));
    } else {
      waiting_text_->set_text(ustring::ascii("Press start or\nspacebar to join"));
    }

    auto colour = kTextColour;
    auto t = frame_++ % 256;
    float a = t < 128 ? t / 128.f : (256 - t) / 128.f;
    colour.a = a;
    waiting_text_->set_colour(colour).set_drop_shadow(kDropShadow, a);
  }

private:
  std::size_t index_ = 0;
  bool joining_ = false;
  bool locked_ = false;
  std::optional<ui::input_device_id> assigned_id_;
  std::optional<ustring> owner_username_;
  std::optional<std::string> controller_name_;

  std::uint32_t frame_ = 0;
  ui::TabContainer* main_ = nullptr;
  ui::TabContainer* config_tab_ = nullptr;
  ui::TextElement* waiting_text_ = nullptr;
  ui::TextElement* ready_text_ = nullptr;
  ui::TextElement* controller_text_ = nullptr;
  ui::Button* ready_button_ = nullptr;
  ui::Button* leave_button_ = nullptr;
};

LobbySlotCoordinator::LobbySlotCoordinator(ui::GameStack& stack, ui::Element& element, bool online)
: stack_{stack}, online_{online} {
  for (std::uint32_t i = 0; i < kMaxPlayers; ++i) {
    slots_.emplace_back().panel = element.add_back<LobbySlotPanel>(i);
  }
}

void LobbySlotCoordinator::set_mode(game_mode mode) {
  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    slots_[i].panel->set_colour(player_colour(mode, i));
  }
}

void LobbySlotCoordinator::handle(const std::vector<data::lobby_update_packet::slot_info>& info,
                                  ui::output_frame& output) {
  if (!is_client()) {
    return;
  }
  for (std::uint32_t i = 0; i < slots_.size() && i < info.size(); ++i) {
    if (slots_[i].owner != info[i].owner_user_id) {
      slots_[i].owner = info[i].owner_user_id;
      output.sounds.emplace(sound::kMenuClick);
    }
    if (slots_[i].owner != stack_.system().local_user().id &&
        slots_[i].is_ready != info[i].is_ready) {
      slots_[i].is_ready = info[i].is_ready;
      output.sounds.emplace(sound::kMenuClick);
    }
  }
}

void LobbySlotCoordinator::handle(std::uint64_t client_user_id,
                                  const data::lobby_request_packet& request,
                                  ui::output_frame& output) {
  if (!is_host()) {
    return;
  }

  auto current_slots = std::count_if(slots_.begin(), slots_.end(),
                                     [&](const auto& s) { return s.owner == client_user_id; });
  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    if (slots_[i].owner != client_user_id) {
      continue;
    }
    auto it = std::find_if(request.slots.begin(), request.slots.end(),
                           [&](const auto& s) { return s.index == i; });
    if (request.slots_requested < current_slots && it == request.slots.end()) {
      slots_[i].owner.reset();
      slots_[i].is_ready = false;
      host_slot_info_dirty_ = true;
      output.sounds.emplace(sound::kMenuClick);
    } else if (it->is_ready != slots_[i].is_ready) {
      slots_[i].is_ready = it->is_ready;
      host_slot_info_dirty_ = true;
      output.sounds.emplace(sound::kMenuClick);
    }
  }

  if (request.slots_requested > current_slots) {
    std::uint32_t slots_requested = request.slots_requested - current_slots;
    for (std::uint32_t i = 0; i < slots_requested; ++i) {
      std::optional<std::uint32_t> index;
      for (std::uint32_t j = 0; j < slots_.size(); ++j) {
        if (!slots_[j].owner) {
          index = j;
          break;
        }
      }
      if (index) {
        slots_[*index].owner = client_user_id;
        slots_[*index].is_ready = false;
        host_slot_info_dirty_ = true;
        output.sounds.emplace(sound::kMenuClick);
      } else {
        break;
      }
    }
  }
}

bool LobbySlotCoordinator::update(const std::vector<ui::input_device_id>& joins) {
  auto owned = [this](const slot_data& slot) {
    return !online_ || slot.owner == stack_.system().local_user().id;
  };

  auto device_in_use = [this](const ui::input_device_id& id) {
    return std::count(queued_devices_.begin(), queued_devices_.end(), id) ||
        std::count_if(slots_.begin(), slots_.end(),
                      [&](const auto& slot) { return slot.assignment == id; });
  };

  auto free_owned_slot = [&]() -> std::optional<std::size_t> {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      if (owned(slots_[i]) && !slots_[i].assignment) {
        return i;
      }
    }
    if (is_host()) {
      for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (!slots_[i].owner) {
          slots_[i].owner = stack_.system().local_user().id;
          host_slot_info_dirty_ = true;
          return i;
        }
      }
    }
    return std::nullopt;
  };

  if (is_host()) {
    for (auto& slot : slots_) {
      if (!slot.owner) {
        continue;
      }
      auto lobby = stack_.system().current_lobby();
      bool in_lobby = lobby &&
          std::count_if(lobby->members.begin(), lobby->members.end(),
                        [&](const auto& m) { return m.id == *slot.owner; });
      if (!in_lobby) {
        slot.owner.reset();
        host_slot_info_dirty_ = true;
      }
    }
  }
  for (const auto& join : joins) {
    if (!locked_ && !device_in_use(join)) {
      queued_devices_.emplace_back(join);
      client_slot_info_dirty_ = true;
    }
  }

  bool joined = false;
  for (auto it = queued_devices_.begin(); it != queued_devices_.end();) {
    auto slot_index = free_owned_slot();
    if (!slot_index) {
      break;
    }
    std::optional<std::string> controller_name;
    if (it->controller_index && *it->controller_index < stack_.io_layer().controllers()) {
      controller_name = stack_.io_layer().controller_info()[*it->controller_index].get_name();
    }
    stack_.input().assign_input_device(*it, static_cast<std::uint32_t>(*slot_index));
    auto& slot = slots_[*slot_index];
    slot.assignment = *it;
    slot.is_ready = false;
    slot.panel->assign(*it, std::move(controller_name));
    it = queued_devices_.erase(it);
    client_slot_info_dirty_ = true;
    host_slot_info_dirty_ = true;
    joined = true;
  };

  std::size_t joining_index = 0;
  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    slots_[i].panel->set_joining(!slots_[i].owner && joining_index++ < queued_devices_.size());
    if (online_ && stack_.system().current_lobby() && slots_[i].owner) {
      for (const auto& m : stack_.system().current_lobby()->members) {
        if (m.id == slots_[i].owner) {
          slots_[i].panel->set_username(m.name);
        }
      }
    }
    if (!owned(slots_[i])) {
      if (!slots_[i].owner) {
        stack_.input().unassign(i);
        slots_[i].assignment.reset();
        slots_[i].panel->clear();
      } else {
        slots_[i].panel->set_remote(slots_[i].is_ready);
      }
      continue;
    }
    if (!slots_[i].panel->is_assigned()) {
      stack_.input().unassign(i);
      slots_[i].assignment.reset();
      slots_[i].owner.reset();
      client_slot_info_dirty_ = true;
      host_slot_info_dirty_ = true;
    }
    if (slots_[i].is_ready != slots_[i].panel->is_ready()) {
      slots_[i].is_ready = slots_[i].panel->is_ready();
      client_slot_info_dirty_ = true;
      host_slot_info_dirty_ = true;
    }
  }

  if (std::all_of(slots_.begin(), slots_.end(),
                  [&](const auto& slot) { return slot.owner || slot.assignment; })) {
    queued_devices_.clear();
    client_slot_info_dirty_ = true;
  }
  return joined;
}

bool LobbySlotCoordinator::game_ready() const {
  std::uint32_t assigned_count = 0;
  std::uint32_t ready_count = 0;
  for (const auto& slot : slots_) {
    if (slot.assignment || slot.owner) {
      ++assigned_count;
      if (slot.is_ready) {
        ++ready_count;
      }
    }
  }
  return assigned_count && assigned_count == ready_count;
}

std::vector<ui::input_device_id> LobbySlotCoordinator::input_devices() const {
  std::vector<ui::input_device_id> input_devices;
  for (const auto& slot : slots_) {
    if (slot.assignment) {
      input_devices.emplace_back(*slot.assignment);
    }
  }
  return input_devices;
}

std::uint32_t LobbySlotCoordinator::player_count() const {
  return static_cast<std::uint32_t>(
      std::count_if(slots_.begin(), slots_.end(),
                    [](const auto& slot) { return slot.assignment || slot.owner; }));
}

void LobbySlotCoordinator::lock() {
  locked_ = true;
  for (const auto& slot : slots_) {
    slot.panel->set_locked(true);
  }
}

void LobbySlotCoordinator::unlock() {
  for (const auto& slot : slots_) {
    slot.panel->set_locked(false);
  }
  locked_ = false;
}

void LobbySlotCoordinator::set_dirty() {
  host_slot_info_dirty_ = true;
  client_slot_info_dirty_ = true;
}

std::optional<data::lobby_request_packet> LobbySlotCoordinator::client_request() {
  if (!is_client() || !client_slot_info_dirty_) {
    return std::nullopt;
  }
  client_slot_info_dirty_ = false;

  data::lobby_request_packet packet;
  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    auto& slot = slots_[i];
    if (slot.owner == stack_.system().local_user().id) {
      ++packet.slots_requested;
      auto& info = packet.slots.emplace_back();
      info.index = i;
      info.is_ready = slot.is_ready;
    }
  }
  packet.slots_requested += queued_devices_.size();
  return {std::move(packet)};
}

std::optional<std::vector<data::lobby_update_packet::slot_info>>
LobbySlotCoordinator::host_slot_update() {
  if (!is_host() || !host_slot_info_dirty_) {
    return std::nullopt;
  }
  host_slot_info_dirty_ = false;

  std::vector<data::lobby_update_packet::slot_info> slot_info;
  for (const auto& slot : slots_) {
    auto& info = slot_info.emplace_back();
    info.owner_user_id = slot.owner;
    info.is_ready = slot.is_ready;
  }
  return {std::move(slot_info)};
}

data::lobby_update_packet::start_game_setup LobbySlotCoordinator::start_game_setup() {
  data::lobby_update_packet::start_game_setup setup;
  for (const auto& slot : slots_) {
    if (slot.owner) {
      setup.players.emplace_back().user_id = *slot.owner;
    }
  }
  return setup;
}

bool LobbySlotCoordinator::is_host() const {
  return online_ && stack_.system().current_lobby() && !stack_.system().current_lobby()->host;
}

bool LobbySlotCoordinator::is_client() const {
  return online_ && stack_.system().current_lobby() && stack_.system().current_lobby()->host;
}

}  // namespace ii