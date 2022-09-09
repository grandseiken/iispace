#include "game/core/ui/element.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

bool Element::focus(bool last) {
  if (+(flags() & element_flags::kCanFocus)) {
    for (auto* e = root(); e;) {
      e->focus_ = false;
      auto* c = e->focused_child_;
      e->focused_child_ = nullptr;
      e = c;
    }
    for (auto* e = this; e; e = e->parent_) {
      e->focus_ = true;
      if (e->parent_) {
        e->parent_->focused_child_ = e;
      }
    }
    return true;
  }
  if (last) {
    return std::find_if(rbegin(), rend(), [](const auto& e) { return e->focus(); }) != rend();
  }
  return std::find_if(begin(), end(), [](const auto& e) { return e->focus(); }) != end();
}

void Element::unfocus() {
  for (auto* e = this; e; e = e->parent_) {
    if (!e->focus_ || (e != this && +(e->flags() & element_flags::kCanFocus))) {
      break;
    }
    e->focus_ = false;
    if (e->parent_) {
      e->parent_->focused_child_ = nullptr;
    }
  }
}

void Element::update(const input_frame& input, output_frame& output) {
  auto element_input = input;
  if (element_input.mouse_cursor) {
    if (bounds().contains(*element_input.mouse_cursor)) {
      *element_input.mouse_cursor -= bounds().min();
    } else {
      element_input.mouse_cursor.reset();
    }
  }
  update_content(element_input, output);
  for (auto& e : children_) {
    e->update(element_input, output);
  }
}

void Element::update_focus(const input_frame& input, output_frame& output) {
  for (auto* e = focused_element(); e; e = e->parent()) {
    if (e->handle_focus(input, output)) {
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