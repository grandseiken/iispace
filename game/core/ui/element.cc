#include "game/core/ui/element.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

bool Element::focus() {
  if (+(flags() & element_flags::kCanFocus)) {
    for (auto* e = root(); e; e = e->focused_child_) {
      e->focus_ = false;
    }
    for (auto* e = this; e; e = e->parent_) {
      e->focus_ = true;
    }
    return true;
  }
  return std::find_if(begin(), end(), [](const auto& e) { return e->focus(); }) != end();
}

void Element::unfocus() {
  for (auto* e = this; e; e = e->parent_) {
    if (!e->focus_ || +(e->flags() & element_flags::kCanFocus)) {
      break;
    }
    e->focus_ = false;
  }
}

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

void Element::update_focus(const input_frame& input) {
  if (!has_focus()) {
    focus();
  }
  for (auto* e = focused_element(); e; e = e->parent()) {
    if (e->handle_focus(input)) {
      break;
    }
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