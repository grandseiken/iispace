#ifndef II_GAME_CORE_UI_ELEMENT_H
#define II_GAME_CORE_UI_ELEMENT_H
#include "game/common/rect.h"
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
  Element(Element* parent) : parent_{parent} {}

  rect bounds() const { return bounds_; }
  void set_bounds(const rect& bounds) { bounds_ = bounds; }

  void remove() { remove_ = true; }
  bool is_removed() const { return remove_; }

  void focus() { focus_internal(true, nullptr); }
  void unfocus() { focus_internal(false, nullptr); }
  bool has_focus() const { return focus_; }
  bool has_primary_focus() const { return focus_ && !focused_child_; }

  Element* parent() { return parent_; }
  const Element* parent() const { return parent_; }

  bool empty() const { return children_.empty(); }
  std::size_t size() const { return children_.size(); }

  auto begin() const { return children_.cbegin(); }
  auto end() const { return children_.cend(); }
  auto begin() { return children_.begin(); }
  auto end() { return children_.end(); }

  template <typename T, typename... Args>
  T* add_back(Args&&... args) {
    auto u = std::make_unique<T>(this, std::forward<Args>(args)...);
    auto* r = u.get();
    children_.emplace_back(std::move(u));
    return r;
  }

  void update(const input_frame&);
  void render(render::GlRenderer&) const;

protected:
  virtual void update_content(const input_frame&) {}
  virtual void render_content(render::GlRenderer&) const {}

private:
  void focus_internal(bool focus, Element* origin);

  rect bounds_;
  bool remove_ = false;
  bool focus_ = false;

  Element* parent_ = nullptr;
  Element* focused_child_ = nullptr;
  std::deque<std::unique_ptr<Element>> children_;
};

}  // namespace ii::ui

#endif