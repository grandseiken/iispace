#include "game/core/toolkit/panel.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

void Panel::update_content(const input_frame&, output_frame&) {
  auto r =
      bounds().size_rect().contract_min(margin_min_).contract_max(margin_max_).contract(padding_);
  for (auto& e : *this) {
    e->set_bounds(r);
  }
}

void Panel::render_content(render::GlRenderer& r) const {
  r.render_panel({
      .style = style_,
      .colour = colour_,
      .border = border_,
      .bounds = frect{bounds().size_rect().contract_min(margin_min_).contract_max(margin_max_)},
  });
}

}  // namespace ii::ui