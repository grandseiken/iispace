#include "game/core/layers/run_lobby.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"
#include "game/logic/sim/io/player.h"

namespace ii {

class AssignmentPanel : public ui::Panel {
public:
  AssignmentPanel(Element* parent, std::size_t index)
  : ui::Panel{parent}
  , index_{index}
  , inactive_{*add_back<ui::Panel>()}
  , active_{*add_back<ui::Panel>()}
  , text_{*inactive_.add_back<ui::TextElement>()} {
    active_.hide();
    text_.set_text(ustring::ascii("Press start or\nspacebar to join"))
        .set_font(render::font_id::kMonospace)
        .set_colour(kTextColour)
        .set_font_dimensions(kMediumFont)
        .set_drop_shadow(kDropShadow, .5f)
        .set_alignment(ui::alignment::kCentered)
        .set_multiline(true);

    auto colour = player_colour(index);
    auto focus_colour = colour;
    focus_colour.z += .125f;

    auto& layout = *active_.add_back<ui::LinearLayout>();
    layout.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);
    controller_text_ = layout.add_back<ui::TextElement>();
    controller_text_->set_alignment(ui::alignment::kCentered)
        .set_colour(colour)
        .set_font(render::font_id::kMonospaceItalic)
        .set_font_dimensions(kMediumFont);
    auto& ready = standard_button(*layout.add_back<ui::Button>())
                      .set_text(ustring::ascii("Ready"))
                      .set_text_colour(colour, focus_colour);
    auto& leave = standard_button(*layout.add_back<ui::Button>())
                      .set_text(ustring::ascii("Leave"))
                      .set_text_colour(colour, focus_colour)
                      .set_callback([this] { clear(); });
    layout.set_absolute_size(ready, kLargeFont.y + 2 * kPadding.y);
    layout.set_absolute_size(leave, kLargeFont.y + 2 * kPadding.y);
  }

  void assign(ui::input_device_id id) {
    assign_input_root(index_);
    inactive_.hide();
    active_.show();
    if (id.controller_index) {
      controller_text_->set_text(ustring::ascii("Controller"));
      active_.focus();
    } else {
      controller_text_->set_text(ustring::ascii("KBM"));
    }
  }

  void clear() {
    if (auto* e = active_.focused_element()) {
      e->unfocus();
    }
    active_.hide();
    inactive_.show();
    clear_input_root();
  }

protected:
  void update_content(const ui::input_frame& input, ui::output_frame& output) override {
    ui::Panel::update_content(input, output);

    if (input.pressed(ui::key::kCancel)) {
      output.sounds.emplace(sound::kMenuAccept);
      clear();
    }

    auto colour = kTextColour;
    auto t = frame_++ % 256;
    float a = t < 128 ? t / 128.f : (256 - t) / 128.f;
    colour.a = a;
    text_.set_colour(colour).set_drop_shadow(kDropShadow, a);
  }

private:
  std::size_t index_ = 0;
  bool assigned_ = false;

  std::uint32_t frame_ = 0;
  ui::Panel& inactive_;
  ui::Panel& active_;
  ui::TextElement& text_;
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
  auto& bottom = *layout.add_back<ui::LinearLayout>();

  top.set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(kBackgroundColour);
  layout.set_absolute_size(top, kLargeFont.y + 2 * kPadding.y)
      .set_absolute_size(bottom, kLargeFont.y + 2 * kPadding.y);
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

  for (std::uint32_t i = 0; i < assignment_panels_.size(); ++i) {
    if (!assignment_panels_[i]->is_input_root()) {
      stack().input().unassign(i);
    }
  }
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
    stack().input().assign_input_device(join, static_cast<std::uint32_t>(*new_index));
    assignment_panels_[*new_index]->assign(join);
    output.sounds.emplace(sound::kMenuAccept);
    back_button_->unfocus();
  }

  if (input.pressed(ui::key::kCancel)) {
    output.sounds.emplace(sound::kMenuAccept);
    stack().input().clear_assignments();
    remove();
  }
}

}  // namespace ii
