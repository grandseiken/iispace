#include "game/core/modal.h"

void Modal::add(Modal* modal) {
  stack_->add(modal);
}

void Modal::quit() {
  quit_ = true;
}

void ModalStack::add(Modal* modal) {
  modal->stack_ = this;
  new_stack_.emplace_back(modal);
}

bool ModalStack::update(Lib& lib) {
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
    (*it++)->update(lib);
  }
  for (auto& modal : new_stack_) {
    stack_.emplace_back(std::move(modal));
  }
  new_stack_.clear();
  return captured;
}

bool ModalStack::render(Lib& lib) const {
  bool captured = false;
  auto it = stack_.begin();
  for (auto jt = it; jt != stack_.end(); ++jt) {
    if ((*jt)->capture_rendering_) {
      it = jt;
      captured = true;
    }
  }
  for (; it != stack_.end(); ++it) {
    (*it)->render(lib);
  }
  return captured;
}