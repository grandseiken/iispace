#include "game/core/sim/sim_layer.h"
#include "game/core/game_options.h"
#include "game/core/sim/input_adapter.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/core/sim/sim_layer_internal.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <vector>

namespace ii {

struct SimLayer::impl_t {
  impl_t(io::IoLayer& io_layer, const initial_conditions& conditions, const game_options_t& options)
  : options{options}
  , mode{conditions.mode}
  , render_state{conditions.seed}
  , input{io_layer}
  , writer{conditions}
  , state{std::make_unique<SimState>(conditions, &writer, options.ai_players)} {
    input.set_player_count(conditions.player_count);
  }

  std::uint32_t audio_tick = 0;
  bool show_controllers_dialog = true;
  bool controllers_dialog = true;

  game_options_t options;
  game_mode mode;
  RenderState render_state;
  InputAdapter input;
  data::ReplayWriter writer;
  std::unique_ptr<SimState> state;
};

SimLayer::~SimLayer() = default;

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   const game_options_t& options)
: ui::GameLayer{stack,
                ui::layer_flag::kCaptureUpdate | ui::layer_flag::kCaptureRender |
                    ui::layer_flag::kCaptureCursor}
, impl_{std::make_unique<impl_t>(stack.io_layer(), conditions, options)} {}

void SimLayer::update_content(const ui::input_frame& ui_input, ui::output_frame&) {
  set_sim_layer(*impl_->state, stack(), *this);
  if (is_removed()) {
    return;
  }
  if (impl_->state->game_over()) {
    end_game();
    stack().play_sound(sound::kMenuAccept);
    remove();
    return;
  }

  if (impl_->show_controllers_dialog) {
    impl_->controllers_dialog = true;
  }
  impl_->show_controllers_dialog = false;

  if (impl_->controllers_dialog) {
    if (ui_input.pressed(ui::key::kStart) || ui_input.pressed(ui::key::kAccept)) {
      impl_->controllers_dialog = false;
      stack().play_sound(sound::kMenuAccept);
    }
    return;
  }

  impl_->input.set_game_dimensions(impl_->state->dimensions());
  auto sim_input = impl_->input.get();
  impl_->state->ai_think(sim_input);
  impl_->state->update(sim_input);

  bool handle_audio = !(impl_->audio_tick++ % 4);
  impl_->render_state.set_dimensions(impl_->state->dimensions());
  impl_->render_state.handle_output(*impl_->state, handle_audio ? &stack().mixer() : nullptr,
                                    &impl_->input);
  impl_->render_state.update(&impl_->input);

  if ((ui_input.pressed(ui::key::kStart) || ui_input.pressed(ui::key::kEscape)) &&
      !impl_->controllers_dialog) {
    stack().add<PauseLayer>([this] {
      end_game();
      remove();
    });
    return;
  }
}

void SimLayer::render_content(render::GlRenderer& r) const {
  const auto& render =
      impl_->state->render(/* paused */ impl_->controllers_dialog || stack().top() != this);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r);  // TODO: can be merged with below?
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes, /* trail alpha */ 1.f);
  render_hud(r, impl_->mode, render);

  if (impl_->controllers_dialog) {
    render_text(r, {4.f, 4.f}, "CONTROLLERS FOUND", kPanelText);

    for (std::size_t i = 0; i < render.players.size(); ++i) {
      std::stringstream ss;
      ss << "PLAYER " << (i + 1) << ": ";
      render_text(r, {4.f, 8.f + 2 * i}, ss.str(), kPanelText);

      ss.str({});
      bool b = false;
      auto input_type = impl_->input.input_type_for(i);
      if (!input_type) {
        ss << "NONE";
      }
      if (input_type & InputAdapter::kController) {
        ss << "GAMEPAD";
        if (input_type & InputAdapter::kKeyboardMouse) {
          ss << ", KB/MOUSE";
        }
        b = true;
      } else if (input_type & InputAdapter::kKeyboardMouse) {
        ss << "MOUSE & KEYBOARD";
        b = true;
      }

      render_text(r, {14.f, 8.f + 2 * i}, ss.str(), b ? player_colour(i) : kPanelText);
    }
    return;
  }
}

void SimLayer::end_game() {
  auto results = impl_->state->results();
  if (impl_->mode == game_mode::kNormal || impl_->mode == game_mode::kBoss) {
    stack().savegame().bosses_killed |= results.bosses_killed();
  } else {
    stack().savegame().hard_mode_bosses_killed |= results.bosses_killed();
  }
  stack().write_savegame();
  stack().write_replay(impl_->writer, "untitled", results.score);
}

}  // namespace ii
