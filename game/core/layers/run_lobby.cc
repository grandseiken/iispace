#include "game/core/layers/run_lobby.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"

namespace ii {

RunLobbyLayer::RunLobbyLayer(ui::GameStack& stack, const initial_conditions& conditions)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer}, conditions_{conditions} {
  set_bounds(rect{kUiDimensions});

  auto& panel = *add_back<ui::Panel>();
  panel.set_padding(kSpacing);
  auto& layout = *panel.add_back<ui::LinearLayout>();
  layout.set_spacing(kPadding.y);

  auto& top = *layout.add_back<ui::Panel>();
  auto& main = *layout.add_back<ui::LinearLayout>();
  auto& bottom = *layout.add_back<ui::Panel>();

  top.set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(kBackgroundColour);
  bottom.set_padding(kPadding);
  layout.set_absolute_size(top, kLargeFont.y + 2 * kPadding.y);
  layout.set_absolute_size(bottom, kLargeFont.y + 2 * kPadding.y);

  auto& title = *top.add_back<ui::TextElement>();
  auto& status = *bottom.add_back<ui::TextElement>();
  status_ = &status;

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

  title.set_text(ustring::ascii(title_text))
      .set_font(render::font_id::kMonospaceBold)
      .set_colour(kHighlightColour)
      .set_font_dimensions(kLargeFont)
      .set_drop_shadow(kDropShadow, .5f)
      .set_alignment(ui::alignment::kCentered);
  status.set_text(ustring::ascii("Press start/enter to join"))
      .set_font(render::font_id::kMonospace)
      .set_colour(kTextColour)
      .set_font_dimensions(kLargeFont)
      .set_drop_shadow(kDropShadow, .5f)
      .set_alignment(ui::alignment::kCentered);
}

void RunLobbyLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  GameLayer::update_content(input, output);

  auto colour = kTextColour;
  auto t = stack().frame() % 256;
  float a = t < 128 ? t / 128.f : (256 - t) / 128.f;
  colour.a = a;
  status_->set_colour(colour).set_drop_shadow(kDropShadow, a);
}

}  // namespace ii
