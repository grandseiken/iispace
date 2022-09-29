#include "game/core/layers/slot_coordinator.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/text.h"
#include "game/core/layers/common.h"
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
    waiting_text_->set_text(ustring::ascii("Press start or\nspacebar to join"))
        .set_font(render::font_id::kMonospaceItalic)
        .set_colour(kTextColour)
        .set_font_dimensions(kSemiLargeFont)
        .set_drop_shadow(kDropShadow, .5f)
        .set_alignment(ui::alignment::kCentered)
        .set_multiline(true);

    auto colour = player_colour(index);
    auto focus_colour = colour;
    focus_colour.z += .125f;

    auto& layout = *active.add_back<ui::LinearLayout>();
    auto& l_top = *layout.add_back<ui::LinearLayout>();

    config_tab_ = layout.add_back<ui::TabContainer>();
    auto& l_bottom = *config_tab_->add_back<ui::LinearLayout>();
    l_bottom.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);
    controller_text_ = l_top.add_back<ui::TextElement>();
    controller_text_->set_alignment(ui::alignment::kCentered)
        .set_colour(colour)
        .set_font(render::font_id::kMonospaceItalic)
        .set_drop_shadow(kDropShadow, .5f)
        .set_font_dimensions(kSemiLargeFont)
        .set_multiline(true);
    auto& ready_button = standard_button(*l_bottom.add_back<ui::Button>())
                             .set_text(ustring::ascii("Ready"))
                             .set_text_colour(colour, focus_colour)
                             .set_callback([this] { ready(); });
    auto& leave_button = standard_button(*l_bottom.add_back<ui::Button>())
                             .set_text(ustring::ascii("Leave"))
                             .set_text_colour(colour, focus_colour)
                             .set_callback([this] { clear(); });
    l_bottom.set_absolute_size(ready_button, kLargeFont.y + 2 * kPadding.y);
    l_bottom.set_absolute_size(leave_button, kLargeFont.y + 2 * kPadding.y);

    auto& bottom_ready = *config_tab_->add_back<ui::TextElement>();
    bottom_ready.set_text(ustring::ascii("Ready"))
        .set_alignment(ui::alignment::kCentered)
        .set_colour(colour)
        .set_font(render::font_id::kMonospaceBoldItalic)
        .set_drop_shadow(kDropShadow, .5f)
        .set_font_dimensions(kLargeFont);
  }

  void assign(ui::input_device_id id) {
    assigned_id_ = id;
    assign_input_root(index_);
    main_->set_active(1);
    config_tab_->set_active(0);
    std::string s = "Player " + std::to_string(index_ + 1) + "\n";
    if (id.controller_index) {
      s += "Controller";
      main_->focus();
    } else {
      s += "Mouse and keyboard";
    }
    controller_text_->set_text(ustring::ascii(s));
  }

  void ready() { config_tab_->set_active(1); }
  void clear() {
    if (auto* e = main_->focused_element()) {
      e->unfocus();
    }
    assigned_id_.reset();
    main_->set_active(0);
    clear_input_root();
  }

  bool is_assigned() const { return assigned_id_.has_value(); }
  bool is_ready() const { return config_tab_->active_index(); }
  ui::input_device_id assigned_input_device() const { return *assigned_id_; }

protected:
  void update_content(const ui::input_frame& input, ui::output_frame& output) override {
    ui::Panel::update_content(input, output);

    if (input.pressed(ui::key::kCancel) && main_->active_index()) {
      if (config_tab_->active_index()) {
        config_tab_->set_active(0);
      } else {
        clear();
      }
      output.sounds.emplace(sound::kMenuAccept);
    }

    auto colour = kTextColour;
    auto t = frame_++ % 256;
    float a = t < 128 ? t / 128.f : (256 - t) / 128.f;
    colour.a = a;
    waiting_text_->set_colour(colour).set_drop_shadow(kDropShadow, a);
  }

private:
  std::size_t index_ = 0;
  std::optional<ui::input_device_id> assigned_id_;

  std::uint32_t frame_ = 0;
  ui::TabContainer* main_;
  ui::TabContainer* config_tab_;
  ui::TextElement* waiting_text_;
  ui::TextElement* controller_text_ = nullptr;
};

LobbySlotCoordinator::LobbySlotCoordinator(ui::GameStack& stack, ui::Element& element, bool online)
: stack_{stack}, online_{online} {
  for (std::uint32_t i = 0; i < kMaxPlayers; ++i) {
    slots_.emplace_back(element.add_back<LobbySlotPanel>(i));
  }
}

void LobbySlotCoordinator::handle(const std::vector<data::lobby_update_packet::slot_info>& info) {
  // TODO
}

bool LobbySlotCoordinator::update(const std::vector<ui::input_device_id>& joins) {
  bool joined = false;
  for (const auto& join : joins) {
    if (stack_.input().is_assigned(join) || std::count(input_devices_.begin(), input_devices_.end(), join)) {
      continue;
    }
    std::optional<std::size_t> new_index;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
      if (!slots_[i]->is_input_root()) {
        new_index = i;
        break;
      }
    }
    if (!new_index) {
      break;
    }
    stack_.input().assign_input_device(join, static_cast<std::uint32_t>(*new_index));
    slots_[*new_index]->assign(join);
    joined = true;
  }

  for (std::uint32_t i = 0; i < slots_.size(); ++i) {
    if (!slots_[i]->is_assigned()) {
      stack_.input().unassign(i);
    }
  }
  return joined;
}

bool LobbySlotCoordinator::game_ready() const {
  std::uint32_t assigned_count = 0;
  std::uint32_t ready_count = 0;
  for (const auto* slot : slots_) {
    if (slot->is_assigned()) {
      ++assigned_count;
      if (slot->is_ready()) {
        ++ready_count;
      }
    }
  }
  return assigned_count && assigned_count == ready_count;
}

std::vector<ui::input_device_id> LobbySlotCoordinator::input_devices() const {
  std::vector<ui::input_device_id> input_devices;
  for (const auto* slot : slots_) {
    if (slot->is_assigned()) {
      input_devices.emplace_back(slot->assigned_input_device());
    }
  }
  return input_devices;
}

std::uint32_t LobbySlotCoordinator::player_count() const {
  return static_cast<std::uint32_t>(std::count_if(
      slots_.begin(), slots_.end(), [](const auto* slot) { return slot->is_assigned(); }));
}

}  // namespace ii