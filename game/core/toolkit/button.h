#ifndef II_GAME_CORE_TOOLKIT_BUTTON_H
#define II_GAME_CORE_TOOLKIT_BUTTON_H
#include "game/common/ustring.h"
#include "game/core/ui/element.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>
#include <functional>

namespace ii::ui {
class Panel;
class TextElement;
enum class alignment : std::uint32_t;

class Button : public Element {
public:
  Button(Element* parent);

  Button& set_callback(std::function<void()> callback) {
    callback_ = std::move(callback);
    return *this;
  }

  Button& set_style(render::panel_style style, render::panel_style focus_style) {
    style_ = style;
    focus_style_ = focus_style;
    return *this;
  }

  Button& set_font(render::font_id font, render::font_id focus_font) {
    font_ = font;
    focus_font_ = focus_font;
    return *this;
  }

  Button& set_colour(const glm::vec4& colour, const glm::vec4& focus_colour) {
    colour_ = colour;
    focus_colour_ = focus_colour;
    return *this;
  }

  Button& set_text_colour(const glm::vec4& colour, const glm::vec4& focus_colour) {
    text_colour_ = colour;
    focus_text_colour_ = focus_colour;
    return *this;
  }

  Button& set_style(render::panel_style style) { return set_style(style, style); }
  Button& set_font(render::font_id font) { return set_font(font, font); }
  Button& set_colour(const glm::vec4& colour) { return set_colour(colour, colour); }
  Button& set_text_colour(const glm::vec4& colour) { return set_text_colour(colour, colour); }

  Button& set_font_dimensions(const glm::uvec2& dimensions);
  Button& set_drop_shadow(const glm::ivec2& offset, float opacity);
  Button& set_alignment(alignment align);
  Button& set_text(ustring&& text);
  Button& set_padding(const glm::ivec2& padding);
  Button& set_margin(const glm::ivec2& margin);

protected:
  void update_content(const input_frame&, output_frame&) override;
  void on_focus_change() override;

private:
  Panel* panel_ = nullptr;
  TextElement* text_element_ = nullptr;
  std::function<void()> callback_;
  bool mouse_over_ = false;

  render::panel_style style_ = render::panel_style::kNone;
  render::panel_style focus_style_ = render::panel_style::kNone;
  render::font_id font_ = render::font_id::kDefault;
  render::font_id focus_font_ = render::font_id::kDefault;
  glm::vec4 colour_{1.f};
  glm::vec4 focus_colour_{1.f};
  glm::vec4 text_colour_{1.f};
  glm::vec4 focus_text_colour_{1.f};
};

}  // namespace ii::ui

#endif
