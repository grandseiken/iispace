#include "game/core/sim/sim_layer.h"
#include "game/core/game_options.h"
#include "game/core/sim/hud_layer.h"
#include "game/core/sim/input_adapter.h"
#include "game/core/sim/pause_layer.h"
#include "game/core/sim/render_state.h"
#include "game/core/ui/input.h"
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
  impl_t(io::IoLayer& io_layer, const initial_conditions& conditions,
         std::span<const ui::input_device_id> input_devices, const game_options_t& options)
  : options{options}
  , mode{conditions.mode}
  , render_state{conditions.seed}
  , input{io_layer, input_devices}
  , writer{conditions}
  , state{std::make_unique<SimState>(conditions, &writer, options.ai_players)} {}

  HudLayer* hud = nullptr;
  std::uint32_t audio_tick = 0;
  game_options_t options;
  game_mode mode;
  RenderState render_state;
  SimInputAdapter input;
  data::ReplayWriter writer;
  std::unique_ptr<SimState> state;
};

SimLayer::~SimLayer() = default;

SimLayer::SimLayer(ui::GameStack& stack, const initial_conditions& conditions,
                   std::span<const ui::input_device_id> input_devices)
: ui::GameLayer{stack, ui::layer_flag::kBaseLayer | ui::layer_flag::kCaptureCursor}
, impl_{std::make_unique<impl_t>(stack.io_layer(), conditions, input_devices, stack.options())} {}

void SimLayer::update_content(const ui::input_frame& ui_input, ui::output_frame&) {
  set_bounds(rect{impl_->state->dimensions()});
  stack().set_fps(impl_->state->fps());
  if (!impl_->hud) {
    impl_->hud = stack().add<HudLayer>(impl_->mode);
  }
  if (is_removed()) {
    return;
  }
  if (impl_->state->game_over()) {
    end_game();
    stack().play_sound(sound::kMenuAccept);
    impl_->hud->remove();
    remove();
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

  if ((ui_input.pressed(ui::key::kStart) || ui_input.pressed(ui::key::kEscape))) {
    stack().add<PauseLayer>([this] {
      end_game();
      impl_->hud->remove();
      remove();
    });
    return;
  }
}

void SimLayer::render_content(render::GlRenderer& r) const {
  const auto& render = impl_->state->render(/* paused */ stack().top() != impl_->hud);
  r.set_colour_cycle(render.colour_cycle);
  impl_->render_state.render(r);  // TODO: can be merged with below?
  r.render_shapes(render::coordinate_system::kGlobal, render.shapes, /* trail alpha */ 1.f);
  impl_->hud->set_data(render);
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
