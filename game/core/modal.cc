#include "game/core/modal.h"

void Modal::quit() {
  quit_ = true;
}

bool ModalStack::update(ii::ui::UiLayer& ui) {
  bool captured = false;
  auto it = stack_.begin();
  for (auto jt = it; jt != stack_.end(); ++jt) {
    if ((*jt)->capture_updates_) {
      it = jt;
      captured = true;
    }
  }
  while (it != stack_.end()) {
    if ((*it)->quit_) {
      it = stack_.erase(it);
      continue;
    }
    (*it++)->update(ui);
  }
  for (auto& modal : new_stack_) {
    stack_.emplace_back(std::move(modal));
  }
  new_stack_.clear();
  return captured;
}

bool ModalStack::render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const {
  bool captured = false;
  auto it = stack_.begin();
  for (auto jt = it; jt != stack_.end(); ++jt) {
    if ((*jt)->capture_rendering_) {
      it = jt;
      captured = true;
    }
  }
  for (; it != stack_.end(); ++it) {
    (*it)->render(ui, r);
  }
  return captured;
}