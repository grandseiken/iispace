#ifndef II_GAME_CORE_UI_ELEMENT_H
#define II_GAME_CORE_UI_ELEMENT_H
#include "game/core/ui/rect.h"
#include <deque>
#include <memory>

namespace ii::render {
class GlRenderer;
}  // namespace ii::render

namespace ii::ui {
struct input_frame;

class Element {
public:
  virtual ~Element() = default;

  rect bounds() const {
    return bounds_;
  }

  void set_bounds(const rect& bounds) {
    bounds_ = bounds;
  }

  void remove() {
    remove_ = true;
  }

  bool is_removed() const {
    return remove_;
  }

  void update(const input_frame&);
  void render(render::GlRenderer&) const;

  template <typename T, typename... Args>
  T* add_back(Args&&... args) {
    return children_.emplace_back(std::make_unique<T>(std::forward<Args>(args)...)).get();
  }

protected:
  virtual void update_content(const input_frame&) {}
  virtual void render_content(render::GlRenderer&) const {}

private:
  bool remove_ = false;
  rect bounds_;
  std::deque<std::unique_ptr<Element>> children_;
};

}  // namespace ii::ui

#endif