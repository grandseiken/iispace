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
  for (const auto& event : stack.system().events()) {
    if (event.type == System::event_type::kOverlayActivated) {
      return true;
    }
  }
  return false;
}

PauseLayer::PauseLayer(ui::GameStack& stack, std::function<void()> on_quit)
: ui::GameLayer{stack, ui::layer_flag::kCaptureUpdate}, on_quit_{std::move(on_quit)} {
  set_bounds(rect{kUiDimensions});

  auto& panel = *add_back<ui::Panel>();
  panel.set_colour({0.f, 0.f, 0.f, 1.f / 2})
      .set_padding({kUiDimensions.x / 4, 3 * kUiDimensions.y / 8})
      .set_style(render::panel_style::kFlatColour);
  auto& inner = *panel.add_back<ui::Panel>();
  inner.set_colour({0.f, 0.f, 0.f, 3.f / 4})
      .set_padding(kPadding)
      .set_style(render::panel_style::kFlatColour);
  auto& layout = *inner.add_back<ui::LinearLayout>();
  layout.set_wrap_focus(true).set_align_end(true).set_spacing(kPadding.y);

  auto add_button = [&](const char* text, std::function<void()> callback) {
    auto& button = *layout.add_back<ui::Button>();
    button.set_callback(std::move(callback))
        .set_text(ustring::ascii(text))
        .set_font(render::font_id::kMonospace, render::font_id::kMonospaceBold)
        .set_text_colour({1.f, 0.f, .65f, 1.f}, glm::vec4{1.f})
        .set_font_dimensions(kLargeFont)
        .set_style(render::panel_style::kFlatColour)
        .set_padding(kPadding)
        .set_colour({1.f, 1.f, 1.f, 1.f / 16})
        .set_drop_shadow(kDropShadow, .5f);
    layout.set_absolute_size(button, 24);
  };

  auto& title = *layout.add_back<ui::TextElement>();
  title.set_text(ustring::ascii("Paused"))
      .set_colour({1.f, 0.f, .5f, 1.f})
      .set_font(render::font_id::kMonospaceItalic)
      .set_font_dimensions(kLargeFont);

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
