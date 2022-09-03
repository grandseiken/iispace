#ifndef II_GAME_CORE_MODAL_H
#define II_GAME_CORE_MODAL_H
#include <memory>
#include <vector>

namespace ii::ui {
class GameStack;
}  // namespace ii::ui
namespace ii::render {
class GlRenderer;
}  // namespace ii::render
class ModalStack;

class Modal {
public:
  Modal(bool capture_updates, bool capture_rendering)
  : capture_updates_{capture_updates}, capture_rendering_{capture_rendering} {}
  virtual ~Modal() {}

  virtual void update(ii::ui::GameStack& ui) = 0;
  virtual void render(const ii::ui::GameStack& ui, ii::render::GlRenderer& r) const = 0;
  virtual std::uint32_t fps() const {
    return 60;
  }

protected:
  template <typename T>
  T* add(std::unique_ptr<T> modal);
  void quit();
  bool is_top() const;

private:
  friend class ModalStack;
  bool capture_updates_ = false;
  bool capture_rendering_ = false;
  bool quit_ = false;
  ModalStack* stack_ = nullptr;
};

class ModalStack {
public:
  template <typename T>
  T* add(std::unique_ptr<T> modal) {
    auto* p = modal.get();
    modal->stack_ = this;
    new_stack_.emplace_back(std::move(modal));
    return p;
  }

  Modal* top() const {
    return new_stack_.empty() ? stack_.back().get() : new_stack_.back().get();
  }

  bool empty() const {
    return stack_.empty() && new_stack_.empty();
  }

  // Returns true if any modal captured the update chain.
  bool update(ii::ui::GameStack& ui);

  // Returns true if any modal captured the render chain.
  bool render(const ii::ui::GameStack& ui, ii::render::GlRenderer& r) const;

private:
  std::vector<std::unique_ptr<Modal>> stack_;
  std::vector<std::unique_ptr<Modal>> new_stack_;
};

template <typename T>
T* Modal::add(std::unique_ptr<T> modal) {
  return stack_->add(std::move(modal));
}

#endif
