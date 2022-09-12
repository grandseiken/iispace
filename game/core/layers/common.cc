#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
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

}  // namespace ii