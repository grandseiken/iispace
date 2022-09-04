#include "game/core/ui/element.h"
#include "game/core/ui/input.h"

namespace ii::ui {

void Element::update(const input_frame& input) {
  update_content(input);
  for (auto& e : children_) {
    e->update(input);
  }
}

void Element::render(render::GlRenderer& renderer) const {
  render_content(renderer);
  for (const auto& e : children_) {
    e->render(renderer);
  }
}

}  // namespace ii::ui