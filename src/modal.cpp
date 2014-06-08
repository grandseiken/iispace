#include "modal.h"

void Modal::add(Modal* modal)
{
  _stack->add(modal);
}

void Modal::quit()
{
  _quit = true;
}

void ModalStack::add(Modal* modal)
{
  modal->_stack = this;
  _new_stack.emplace_back(modal);
}

bool ModalStack::update(Lib& lib)
{
  bool captured = false;
  auto it = _stack.begin();
  for (auto jt = it; jt != _stack.end(); ++jt) {
    if ((*jt)->_capture_updates) {
      it = jt;
      captured = true;
    }
  }
  while (it != _stack.end()) {
    if ((*it)->_quit) {
      it = _stack.erase(it);
      continue;
    }
    (*it++)->update(lib);
  }
  for (auto& modal : _new_stack) {
    _stack.emplace_back(std::move(modal));
  }
  _new_stack.clear();
  return captured;
}

bool ModalStack::render(Lib& lib) const
{
  bool captured = false;
  auto it = _stack.begin();
  for (auto jt = it; jt != _stack.end(); ++jt) {
    if ((*jt)->_capture_rendering) {
      it = jt;
      captured = true;
    }
  }
  for (; it != _stack.end(); ++it) {
    (*it)->render(lib);
  }
  return captured;
}