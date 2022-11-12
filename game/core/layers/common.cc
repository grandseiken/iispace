#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/render/render_common.h"

namespace ii {

ui::Button& standard_button(ui::Button& button) {
  return button.set_font(render::font_id::kMonospace, render::font_id::kMonospaceBold)
      .set_text_colour(kTextColour, kHighlightColour)
      .set_font_dimensions(kLargeFont)
      .set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(kBackgroundColour)
      .set_drop_shadow(kDropShadow, .5f);
}

ui::LinearLayout& add_dialog_layout(ui::Element& element) {
  auto& panel = *element.add_back<ui::Panel>();
  panel.set_colour({0.f, 0.f, 0.f, 1.f / 4})
      .set_padding({kUiDimensions.x / 4, 3 * kUiDimensions.y / 8})
      .set_style(render::panel_style::kFlatColour);
  auto& inner = *panel.add_back<ui::Panel>();
  inner.set_colour({0.f, 0.f, 0.f, 3.f / 8})
      .set_padding(kPadding)
      .set_style(render::panel_style::kFlatColour);
  auto& layout = *inner.add_back<ui::LinearLayout>();
  return layout.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);
}

}  // namespace ii