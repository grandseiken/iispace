#include "game/core/ui/element.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

void Element::update(const input_frame& input) {
  auto element_input = input;
  if (element_input.mouse_cursor) {
    if (bounds().contains(*element_input.mouse_cursor)) {
      *element_input.mouse_cursor -= bounds().min();
    } else {
      element_input.mouse_cursor.reset();
    }
  }
  update_content(element_input);
  for (auto& e : children_) {
    e->update(element_input);
  }
}

void Element::render(render::GlRenderer& renderer) const {
  render_content(renderer);
  for (const auto& e : children_) {
    render::clip_handle clip{renderer.target(), e->bounds()};
    e->render(renderer);
  }
}

}  // namespace ii::ui