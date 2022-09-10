
#include "game/core/layers/main_menu.h"
#include "game/core/layers/common.h"
#include "game/core/sim/sim_layer.h"
#include "game/core/toolkit/button.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/io/io.h"

namespace ii {

MainMenuLayer::MainMenuLayer(ui::GameStack& stack, const game_options_t& options)
: ui::GameLayer{stack}, options_{options} {
  set_bounds(rect{kUiDimensions});
  auto& panel = *add_back<ui::Panel>();
  panel.set_padding(kSpacing);
  auto& layout = *panel.add_back<ui::LinearLayout>();
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
        .set_colour({1.f, 1.f, 1.f, 1.f / 8})
        .set_drop_shadow(kDropShadow, .5f);
    layout.set_absolute_size(button, 24);
  };

  auto start_game = [this](game_mode mode) {
    initial_conditions conditions;
    conditions.compatibility = options_.compatibility;
    conditions.seed = static_cast<std::uint32_t>(time(0));
    conditions.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
    conditions.player_count = 1u /* TODO */;
    conditions.mode = mode;
    this->stack().add<SimLayer>(conditions, options_);
  };

  add_button("Normal mode", [=] { start_game(game_mode::kNormal); });
  add_button("Boss mode", [=] { start_game(game_mode::kBoss); });
  add_button("Hard mode", [=] { start_game(game_mode::kHard); });
  add_button("Fast mode", [=] { start_game(game_mode::kFast); });
  add_button("W-hat mode", [=] { start_game(game_mode::kWhat); });
  add_button("Exit", [this] { exit_timer_ = 4; });

  stack.set_volume(.5f);
  focus();
}

void MainMenuLayer::update_content(const ui::input_frame& input, ui::output_frame& output) {
  ui::GameLayer::update_content(input, output);
  stack().set_fps(60);
  stack().io_layer().capture_mouse(false);
  if (exit_timer_) {
    if (!--exit_timer_) {
      remove();
    }
  }
}

}  // namespace ii
