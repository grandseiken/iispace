#include "game/core/layers/common.h"
#include "game/common/colour.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/render/data/panel.h"

namespace ii {

ui::Button& standard_button(ui::Button& button) {
  return button
      .set_font({render::font_id::kMonospace, kLargeFont},
                {render::font_id::kMonospaceBold, kLargeFont})
      .set_text_colour(kTextColour, kHighlightColour)
      .set_style(render::panel_style::kFlatColour)
      .set_padding(kPadding)
      .set_colour(colour::ui::kBlackOverlay0)
      .set_drop_shadow({});
}

ui::LinearLayout& add_dialog_layout(ui::Element& element) {
  auto& panel = *element.add_back<ui::Panel>();
  panel.set_colour(colour::ui::kBlackOverlay0)
      .set_padding({kUiDimensions.x / 4, 3 * kUiDimensions.y / 8})
      .set_style(render::panel_style::kFlatColour);
  auto& inner = *panel.add_back<ui::Panel>();
  inner.set_colour(colour::ui::kBlackOverlay1)
      .set_padding(kPadding)
      .set_style(render::panel_style::kFlatColour);
  auto& layout = *inner.add_back<ui::LinearLayout>();
  return layout.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);
}

}  // namespace ii