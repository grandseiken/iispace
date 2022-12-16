#include "game/core/sim/pause_layer.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"
#include "game/render/gl_renderer.h"
#include "game/system/system.h"

namespace ii {

bool sim_should_pause(ui::GameStack& stack) {
  return event_triggered(stack.system(), System::event_type::kOverlayActivated).has_value();
}

PauseLayer::PauseLayer(ui::GameStack& stack, std::function<void()> on_quit)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate}, on_quit_{std::move(on_quit)} {
  set_bounds(rect{kUiDimensions});

  auto& layout = add_dialog_layout(*this);
  auto add_button = [&](const char* text, std::function<void()> callback) {
    auto& button = *layout.add_back<ui::Button>();
    standard_button(button).set_text(ustring::ascii(text)).set_callback(std::move(callback));
    layout.set_absolute_size(button, kLargeFont.y + 2 * kPadding.y);
  };

  auto& title = *layout.add_back<ui::TextElement>();
  title.set_text(ustring::ascii("Paused"))
      .set_colour({1.f, 0.f, .5f, 1.f})
      .set_font({render::font_id::kMonospaceItalic, kLargeFont});

  // TODO: reinstate volume control / settings menu. Allow explicit controller rebind?
  add_button("Continue", [this] { remove(); });
  add_button("End game", [this] {
    if (on_quit_) {
      on_quit_();
    }
    remove();
  });

  stack.play_sound(sound::kMenuAccept);
}

void PauseLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  ui::GameLayer::update_content(input, output);

  if (input.pressed(ui::key::kCancel) || input.pressed(ui::key::kEscape)) {
    stack().play_sound(sound::kMenuAccept);
    remove();
  }
}

}  // namespace ii
