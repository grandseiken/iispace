#include "game/core/toolkit/button.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/core/ui/input.h"
#include "game/render/gl_renderer.h"

namespace ii::ui {

Button::Button(Element* parent)
: Element{parent}, panel_{add_back<Panel>()}, text_element_{panel_->add_back<TextElement>()} {
  add_flags(element_flags::kCanFocus);
}

Button& Button::set_font_dimensions(const glm::uvec2& dimensions) {
  text_element_->set_font_dimensions(dimensions);
  return *this;
}

Button& Button::set_text(ustring&& text) {
  text_element_->set_text(std::move(text));
  return *this;
}

Button& Button::set_padding(const glm::ivec2& padding) {
  panel_->set_padding(padding);
  return *this;
}

Button& Button::set_margin(const glm::ivec2& margin) {
  panel_->set_margin(margin);
  return *this;
}

void Button::update_content(const input_frame& input, ui::output_frame& output) {
  if (has_primary_focus()) {
    panel_->set_colour(focus_colour_);
    panel_->set_style(focus_style_);
    text_element_->set_colour(focus_text_colour_);
    text_element_->set_font(focus_font_);

    if (callback_ && input.pressed(ui::key::kAccept)) {
      output.sounds.emplace_back(sound::kMenuAccept);
      callback_();
    }
  } else {
    panel_->set_colour(colour_);
    panel_->set_style(style_);
    text_element_->set_colour(text_colour_);
    text_element_->set_font(font_);
  }
  panel_->set_bounds(bounds().size_rect());
}

}  // namespace ii::ui