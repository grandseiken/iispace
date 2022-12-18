#ifndef II_GAME_CORE_TOOLKIT_BUTTON_H
#define II_GAME_CORE_TOOLKIT_BUTTON_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/common/ustring.h"
#include "game/core/ui/element.h"
#include "game/render/data/panel.h"
#include "game/render/data/text.h"
#include <functional>

namespace ii::ui {
class Panel;
class TextElement;

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

  Button& set_font(const render::font_data& font, const render::font_data& focus_font) {
    font_ = font;
    focus_font_ = focus_font;
    return *this;
  }

  Button& set_colour(const cvec4& colour, const cvec4& focus_colour) {
    colour_ = colour;
    focus_colour_ = focus_colour;
    return *this;
  }

  Button& set_text_colour(const cvec4& colour, const cvec4& focus_colour) {
    text_colour_ = colour;
    focus_text_colour_ = focus_colour;
    return *this;
  }

  Button& set_style(render::panel_style style) { return set_style(style, style); }
  Button& set_font(const render::font_data& font) { return set_font(font, font); }
  Button& set_colour(const cvec4& colour) { return set_colour(colour, colour); }
  Button& set_text_colour(const cvec4& colour) { return set_text_colour(colour, colour); }

  Button& set_drop_shadow(const render::drop_shadow& shadow);
  Button& set_alignment(render::alignment align);
  Button& set_text(ustring&& text);
  Button& set_padding(const ivec2& padding);
  Button& set_margin(const ivec2& margin);

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
  render::font_data font_;
  render::font_data focus_font_;
  cvec4 colour_ = colour::kWhite0;
  cvec4 focus_colour_ = colour::kWhite0;
  cvec4 text_colour_ = colour::kWhite0;
  cvec4 focus_text_colour_ = colour::kWhite0;
};

}  // namespace ii::ui

#endif
