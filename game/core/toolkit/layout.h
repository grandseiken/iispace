#ifndef II_GAME_CORE_TOOLKIT_LAYOUT_H
#define II_GAME_CORE_TOOLKIT_LAYOUT_H
#include "game/core/ui/element.h"
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace ii::ui {

enum class orientation {
  kVertical,
  kHorizontal,
};

class LinearLayout : public Element {
public:
  using Element::Element;

  LinearLayout& set_wrap_focus(bool wrap_focus) {
    wrap_focus_ = wrap_focus;
    return *this;
  }

  LinearLayout& set_orientation(orientation type) {
    type_ = type;
    return *this;
  }

  LinearLayout& set_spacing(std::uint32_t spacing) {
    spacing_ = static_cast<std::int32_t>(spacing);
    return *this;
  }

  LinearLayout& set_absolute_size(const Element& child, std::uint32_t size) {
    info_[&child] = element_info{static_cast<std::int32_t>(size), std::nullopt};
    return *this;
  }

  LinearLayout& set_relative_weight(const Element& child, float weight) {
    info_[&child] = element_info{std::nullopt, weight};
    return *this;
  }

protected:
  void update_content(const input_frame&, ui::output_frame&) override;
  bool handle_focus(const input_frame&, ui::output_frame&) override;

private:
  struct element_info {
    std::optional<std::int32_t> absolute;
    std::optional<float> relative_weight;
  };

  bool wrap_focus_ = false;
  orientation type_ = orientation::kVertical;
  std::int32_t spacing_ = 0;
  std::unordered_map<const Element*, element_info> info_;
};

class GridLayout : public Element {
public:
  using Element::Element;

  GridLayout& set_wrap_focus(bool wrap_focus) {
    wrap_focus_ = wrap_focus;
    return *this;
  }

  GridLayout& set_columns(std::uint32_t columns) {
    columns_ = std::max<std::int32_t>(1, static_cast<std::int32_t>(columns));
    return *this;
  }

  GridLayout& set_spacing(std::uint32_t spacing) {
    spacing_ = static_cast<std::int32_t>(spacing);
    return *this;
  }

protected:
  void update_content(const input_frame&, ui::output_frame&) override;
  bool handle_focus(const input_frame&, ui::output_frame&) override;

private:
  bool wrap_focus_ = false;
  std::int32_t columns_ = 1;
  std::int32_t spacing_ = 0;
};

}  // namespace ii::ui

#endif
