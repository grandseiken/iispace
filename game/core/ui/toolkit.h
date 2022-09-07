#ifndef II_GAME_CORE_UI_TOOLKIT_H
#define II_GAME_CORE_UI_TOOLKIT_H
#include "game/common/ustring.h"
#include "game/core/ui/element.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>

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
  LinearLayout(Element* parent, orientation) : Element{parent} {}

protected:
  void update_content(const input_frame&) override {}
};

class GridLayout : public Element {
public:
  GridLayout(Element* parent, std::uint32_t /* columns */) : Element{parent} {}

protected:
  void update_content(const input_frame&) override {}
};

}  // namespace ii::ui

#endif
