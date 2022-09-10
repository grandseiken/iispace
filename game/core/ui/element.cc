#include "game/core/ui/element.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"
#include <algorithm>

namespace ii::ui {
namespace {

multi_input_frame map_input(const multi_input_frame& input,
                            std::optional<std::size_t> input_assignment, const rect& bounds) {
  auto result = input;
  if (input_assignment) {
    if (*input_assignment < result.assignments.size()) {
      result.global = result.assignments[*input_assignment];
    } else {
      result.global = {};
    }
    result.assignments.clear();
  }
  auto map_cursor = [&](input_frame& frame) {
    if (frame.mouse_cursor) {
      if (bounds.contains(*frame.mouse_cursor)) {
        *frame.mouse_cursor -= bounds.min();
      } else {
        frame.mouse_cursor.reset();
      }
    }
  };
  map_cursor(result.global);
  std::for_each(result.assignments.begin(), result.assignments.end(), map_cursor);
  return result;
}

}  // namespace

bool Element::focus(bool last, std::optional<glm::ivec2> cursor) {
  if (cursor) {
    if (!bounds().contains(*cursor)) {
      return false;
    }
    *cursor -= bounds().min();
  }
  if (+(flags() & element_flags::kCanFocus)) {
    std::vector<Element*> callbacks;
    for (auto* e = root(); e;) {
      callbacks.emplace_back(e);
      e->focus_ = false;
      auto* c = e->focused_child_;
      e->focused_child_ = nullptr;
      e = c;
    }
    for (auto* e = this; e; e = e->parent_) {
      callbacks.emplace_back(e);
      e->focus_ = true;
      if (e->input_root_) {
        break;
      }
      if (e->parent_) {
        e->parent_->focused_child_ = e;
      }
    }
    for (auto* e : callbacks) {
      e->on_focus_change();
    }
    return true;
  }
  auto recurse = [&](const auto& e) { return e->is_visible() && e->focus(last, cursor); };
  return last ? std::find_if(rbegin(), rend(), recurse) != rend()
              : std::find_if(begin(), end(), recurse) != end();
}

void Element::unfocus() {
  std::vector<Element*> callbacks;
  for (auto* e = this; e; e = e->parent_) {
    callbacks.emplace_back(e);
    if (!e->focus_ || (e != this && +(e->flags() & element_flags::kCanFocus))) {
      break;
    }
    e->focus_ = false;
    if (e->input_root_) {
      break;
    }
    if (e->parent_) {
      e->parent_->focused_child_ = nullptr;
    }
  }
  for (auto* e : callbacks) {
    e->on_focus_change();
  }
}

void Element::update(const multi_input_frame& input, output_frame& output) {
  if (!is_visible()) {
    return;
  }
  auto element_input = map_input(input, input_root_, bounds());
  update_content(element_input.global, output);
  for (auto& e : children_) {
    e->update(element_input, output);
  }
}

void Element::update_focus(const multi_input_frame& input, output_frame& output) {
  if (!is_visible()) {
    return;
  }

  if (parent_ == nullptr || input_root_) {
    auto element_input = map_input(input, input_root_, {});
    for (auto* e = focused_element(); e; e = e->parent()) {
      if (e->handle_focus(element_input.global, output)) {
        break;
      }
    }
  }
  for (auto& e : children_) {
    e->update_focus(input, output);
  }
}

void Element::render(render::GlRenderer& renderer) const {
  if (!is_visible()) {
    return;
  }
  render_content(renderer);
  for (const auto& e : children_) {
    render::clip_handle clip{renderer.target(), e->bounds()};
    e->render(renderer);
  }
}

}  // namespace ii::ui