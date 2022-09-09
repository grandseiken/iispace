#ifndef II_GAME_CORE_UI_TOOLKIT_H
#define II_GAME_CORE_UI_TOOLKIT_H
#include "game/common/ustring.h"
#include "game/core/ui/element.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>
#include <optional>
#include <unordered_map>

namespace ii::ui {

enum class orientation {
  kVertical,
  kHorizontal,
};

class Panel : public Element {
public:
  using Element::Element;

  Panel& set_style(render::panel_style style) {
    style_ = style;
    return *this;
  }

  Panel& set_colour(const glm::vec4& colour) {
    colour_ = colour;
    return *this;
  }

  Panel& set_padding(const glm::ivec2& padding) {
    padding_ = padding;
    return *this;
  }

  Panel& set_margin(const glm::ivec2& margin) {
    margin_ = margin;
    return *this;
  }

protected:
  void update_content(const input_frame&) override;
  void render_content(render::GlRenderer&) const override;

private:
  render::panel_style style_ = render::panel_style::kNone;
  glm::vec4 colour_{1.f};
  glm::ivec2 padding_{0};
  glm::ivec2 margin_{0};
};

class TextElement : public Element {
public:
  using Element::Element;

  TextElement& set_font(render::font_id font) {
    font_ = font;
    return *this;
  }

  TextElement& set_font_dimensions(const glm::uvec2& dimensions) {
    font_dimensions_ = dimensions;
    return *this;
  }

  TextElement& set_colour(const glm::vec4& colour) {
    colour_ = colour;
    return *this;
  }

  TextElement& set_multiline(bool multiline) {
    multiline_ = multiline;
    return *this;
  }

  TextElement& set_text(ustring&& text) {
    text_ = std::move(text);
    return *this;
  }

protected:
  void render_content(render::GlRenderer&) const override;

private:
  render::font_id font_ = render::font_id::kDefault;
  glm::uvec2 font_dimensions_{0};
  glm::vec4 colour_{1.f};
  bool multiline_ = false;
  ustring text_ = ustring::ascii("");
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

  LinearLayout& set_absolute_size(const Element* child, std::uint32_t size) {
    info_[child] = element_info{static_cast<std::int32_t>(size), std::nullopt};
    return *this;
  }

  LinearLayout& set_relative_weight(const Element* child, float weight) {
    info_[child] = element_info{std::nullopt, weight};
    return *this;
  }

protected:
  void update_content(const input_frame&) override;
  bool handle_focus(const input_frame&) override;

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
  void update_content(const input_frame&) override;
  bool handle_focus(const input_frame&) override;

private:
  bool wrap_focus_ = false;
  std::int32_t columns_ = 1;
  std::int32_t spacing_ = 0;
};

}  // namespace ii::ui

#endif
