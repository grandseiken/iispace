#include "game/core/layers/utility.h"

namespace ii {

ErrorLayer::ErrorLayer(ui::GameStack& stack, ustring message, std::function<void()> on_close)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate}, on_close_{std::move(on_close)} {
  set_bounds(rect{kUiDimensions});

  auto& layout = add_dialog_layout(*this);
  auto add_button = [&](const char* text, std::function<void()> callback) {
    auto& button = *layout.add_back<ui::Button>();
    standard_button(button).set_text(ustring::ascii(text)).set_callback(std::move(callback));
    layout.set_absolute_size(button, kLargeFont.y + 2 * kPadding.y);
  };

  auto& title = *layout.add_back<ui::TextElement>();
  title.set_text(ustring::ascii("Error"))
      .set_colour(kErrorColour)
      .set_font(render::font_id::kMonospaceBold)
      .set_font_dimensions(kLargeFont);
  layout.set_absolute_size(title, kLargeFont.y);

  auto& message_text = *layout.add_back<ui::TextElement>();
  message_text.set_text(std::move(message))
      .set_colour(kTextColour)
      .set_font(render::font_id::kMonospace)
      .set_font_dimensions(kMediumFont)
      .set_multiline(true);

  add_button("Ok", [this] {
    if (on_close_) {
      on_close_();
    }
    remove();
  });
  stack.play_sound(sound::kPlayerShield);
}

void ErrorLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  ui::GameLayer::update_content(input, output);

  if (input.pressed(ui::key::kCancel) || input.pressed(ui::key::kEscape)) {
    stack().play_sound(sound::kMenuAccept);
    if (on_close_) {
      on_close_();
    }
    remove();
  }
}

}  // namespace ii
