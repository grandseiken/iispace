#include "game/core/layers/run_lobby.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"
#include "game/logic/sim/io/player.h"

namespace ii {
namespace {
constexpr std::uint32_t kAllReadyTimerFrames = 300;
}  // namespace

class AssignmentPanel : public ui::Panel {
public:
  AssignmentPanel(Element* parent, std::size_t index)
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

RunLobbyLayer::RunLobbyLayer(ui::GameStack& stack, const initial_conditions& conditions)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer | ui::layer_flag::kNoAutoFocus}
, conditions_{conditions} {
  set_bounds(rect{kUiDimensions});

  auto& panel = *add_back<ui::Panel>();
  panel.set_padding(kSpacing);
  auto& layout = *panel.add_back<ui::LinearLayout>();
  layout.set_spacing(kPadding.y);

  auto& top = *layout.add_back<ui::Panel>();
  auto& main = *layout.add_back<ui::LinearLayout>();
  bottom_tabs_ = layout.add_back<ui::TabContainer>();
  auto& bottom = *bottom_tabs_->add_back<ui::LinearLayout>();
  all_ready_text_ = bottom_tabs_->add_back<ui::TextElement>();

  top.set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(kBackgroundColour);
  layout.set_absolute_size(top, kLargeFont.y + 2 * kPadding.y)
      .set_absolute_size(*bottom_tabs_, kLargeFont.y + 2 * kPadding.y);
  bottom.set_spacing(kPadding.x).set_orientation(ui::orientation::kHorizontal);

  std::string title_text;
  switch (conditions.mode) {
  default:
  case game_mode::kNormal:
    title_text = "Normal mode";
    break;
  case game_mode::kBoss:
    title_text = "Boss mode";
    break;
  case game_mode::kHard:
    title_text = "Hard mode";
    break;
  case game_mode::kFast:
    title_text = "Fast mode";
    break;
  case game_mode::kWhat:
    title_text = "W-hat mode";
    break;
  }

  auto& title = *top.add_back<ui::TextElement>();
  title.set_text(ustring::ascii(title_text))
      .set_font(render::font_id::kMonospaceBoldItalic)
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
  standard_button(*back_button_).set_text(ustring::ascii("Back")).set_callback([this] {
    this->stack().input().clear_assignments();
    remove();
  });

  main.set_orientation(ui::orientation::kHorizontal);
  main.set_spacing(kPadding.x);
  for (std::uint32_t i = 0; i < kMaxPlayers; ++i) {
    assignment_panels_.emplace_back(main.add_back<AssignmentPanel>(i));
  }
}

void RunLobbyLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  GameLayer::update_content(input, output);

  for (const auto& join : input.join_game_inputs) {
    if (stack().input().is_assigned(join)) {
      continue;
    }
    std::optional<std::size_t> new_index;
    for (std::size_t i = 0; i < assignment_panels_.size(); ++i) {
      if (!assignment_panels_[i]->is_input_root()) {
        new_index = i;
        break;
      }
    }
    if (!new_index) {
      break;
    }
    if (!join.controller_index) {
      stack().set_cursor_hue(player_colour(*new_index).x);
    }
    stack().input().assign_input_device(join, static_cast<std::uint32_t>(*new_index));
    assignment_panels_[*new_index]->assign(join);
    output.sounds.emplace(sound::kMenuAccept);
    back_button_->unfocus();
  }

  std::uint32_t assigned_count = 0;
  std::uint32_t ready_count = 0;
  for (std::uint32_t i = 0; i < assignment_panels_.size(); ++i) {
    auto& panel = *assignment_panels_[i];
    if (!panel.is_assigned()) {
      stack().input().unassign(i);
    }
    if (panel.is_assigned()) {
      ++assigned_count;
      if (panel.is_ready()) {
        ++ready_count;
      }
    }
  }

  bool all_ready = assigned_count && assigned_count == ready_count;
  if (all_ready) {
    if (!all_ready_timer_) {
      all_ready_timer_ = kAllReadyTimerFrames;
    }
    if (!--*all_ready_timer_) {
      all_ready_timer_.reset();
    }
  } else {
    all_ready_timer_.reset();
  }
  if (all_ready_timer_) {
    auto s = "Game starting in " + std::to_string(*all_ready_timer_ / 60);
    bottom_tabs_->set_active(1);
    all_ready_text_->set_text(ustring::ascii(s));
  } else {
    bottom_tabs_->set_active(0);
  }

  if (!stack().input().is_assigned(ui::input_device_id::kbm())) {
    stack().clear_cursor_hue();
  }

  if (input.pressed(ui::key::kCancel)) {
    output.sounds.emplace(sound::kMenuAccept);
    stack().input().clear_assignments();
    stack().clear_cursor_hue();
    remove();
  }
}

}  // namespace ii
