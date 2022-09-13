#ifndef II_GAME_CORE_UI_ELEMENT_H
#define II_GAME_CORE_UI_ELEMENT_H
#include "game/common/enum.h"
#include "game/common/rect.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>

namespace ii::render {
class GlRenderer;
}  // namespace ii::render

namespace ii::ui {
struct multi_input_frame;
struct input_frame;
struct output_frame;
enum class element_flags : std::uint32_t {
  kNone = 0b0000,
  kCanFocus = 0b0001,
};
}  // namespace ii::ui

namespace ii {
class Mixer;
template <>
struct bitmask_enum<ui::element_flags> : std::true_type {};
}  // namespace ii

namespace ii::ui {

class Element {
public:
  virtual ~Element() = default;
  Element(Element* parent) : parent_{parent} {}

  element_flags flags() const { return flags_; }
  void add_flags(element_flags flags) { flags_ |= flags; }

  rect bounds() const { return bounds_; }
  void set_bounds(const rect& bounds) { bounds_ = bounds; }

  void remove() { remove_ = true; }
  bool is_removed() const { return remove_; }

  void show() { hide_ = false; }
  void hide() {
    unfocus();
    hide_ = true;
  }
  bool is_visible() const { return !hide_; }

  bool focus(bool last = false);
  void unfocus();
  bool has_focus() const { return focus_; }
  bool has_primary_focus() const { return focus_ && !focused_child_; }

  Element* parent() { return parent_; }
  Element* focused_child() { return focused_child_; }
  Element* focused_element() {
    return !focus_ ? nullptr : focused_child_ ? focused_child_->focused_element() : this;
  }

  const Element* parent() const { return parent_; }
  const Element* focused_child() const { return focused_child_; }
  const Element* focused_element() const {
    return !focus_ ? nullptr : focused_child_ ? focused_child_->focused_element() : this;
  }

  void assign_input_root(std::size_t index) { input_root_ = index; }
  bool is_input_root() { return input_root_.has_value(); }
  void clear_input_root() { input_root_.reset(); }

  Element* root() {
    auto* e = this;
    while (!e->input_root_ && e->parent()) {
      e = e->parent();
    }
    return e;
  }

  const Element* root() const {
    const auto* e = this;
    while (!e->input_root_ && e->parent()) {
      e = e->parent();
    }
    return e;
  }

  bool empty() const { return children_.empty(); }
  std::size_t size() const { return children_.size(); }
  Element* operator[](std::size_t i) { return children_[i].get(); }
  const Element* operator[](std::size_t i) const { return children_[i].get(); }

  auto rbegin() const { return children_.crbegin(); }
  auto begin() const { return children_.cbegin(); }
  auto rend() const { return children_.crend(); }
  auto end() const { return children_.cend(); }
  auto rbegin() { return children_.rbegin(); }
  auto begin() { return children_.begin(); }
  auto rend() { return children_.rend(); }
  auto end() { return children_.end(); }

  template <typename T, typename... Args>
  T* add_back(Args&&... args) {
    auto u = std::make_unique<T>(this, std::forward<Args>(args)...);
    auto* r = u.get();
    children_.emplace_back(std::move(u));
    return r;
  }

  void update(const multi_input_frame&, output_frame&);
  void update_focus(const multi_input_frame&, output_frame&);
  void render(render::GlRenderer&) const;

protected:
  virtual void update_content(const input_frame&, output_frame&) {}
  virtual bool handle_focus(const input_frame&, output_frame&) { return false; }
  virtual void render_content(render::GlRenderer&) const {}
  virtual void on_focus_change() {}

private:
  element_flags flags_ = element_flags::kNone;
  rect bounds_;
  bool remove_ = false;
  bool hide_ = false;
  bool focus_ = false;

  std::optional<std::size_t> input_root_;
  Element* parent_ = nullptr;
  Element* focused_child_ = nullptr;
  std::deque<std::unique_ptr<Element>> children_;
};

}  // namespace ii::ui

#endif