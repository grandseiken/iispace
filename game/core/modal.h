#ifndef II_GAME_CORE_MODAL_H
#define II_GAME_CORE_MODAL_H
#include <memory>
#include <vector>

namespace ii::ui {
class UiLayer;
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

  virtual void update(ii::ui::UiLayer& ui) = 0;
  virtual void render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const = 0;

protected:
  template <typename T>
  T* add(std::unique_ptr<T> modal) {
    return stack_->add(std::move(modal));
  }
  void quit();

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

  bool empty() const {
    return stack_.empty() && new_stack_.empty();
  }

  // Returns true if any modal captured the update chain.
  bool update(ii::ui::UiLayer& ui);

  // Returns true if any modal captured the render chain.
  bool render(const ii::ui::UiLayer& ui, ii::render::GlRenderer& r) const;

private:
  std::vector<std::unique_ptr<Modal>> stack_;
  std::vector<std::unique_ptr<Modal>> new_stack_;
};

#endif