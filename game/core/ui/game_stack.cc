#include "game/core/ui/game_stack.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ii::ui {
namespace {
const char* kSaveName = "space";
constexpr std::uint32_t kCursorFrames = 32u;

template <typename It>
auto get_capture_it(It begin, It end, layer_flag flag) {
  auto it = end;
  while (it != begin) {
    --it;
    if (+((*it)->layer_flags() & flag)) {
      break;
    }
  }
  return it;
}

}  // namespace

void GameLayer::update_content(const input_frame&, output_frame&) {
  for (auto& e : *this) {
    e->set_bounds(bounds());
  }
}

GameStack::GameStack(io::Filesystem& fs, io::IoLayer& io_layer, System& system, Mixer& mixer,
                     const game_options_t& options)
: fs_{fs}
, io_layer_{io_layer}
, system_{system}
, mixer_{mixer}
, adapter_{io_layer}
, options_{options} {
  auto data = fs.read_config();
  if (data) {
    auto config = data::read_config(*data);
    if (config) {
      config_ = *config;
    }
  }
  data = fs.read_savegame(kSaveName);
  if (data) {
    auto save_data = data::read_savegame(*data);
    if (save_data) {
      save_ = std::move(*save_data);
    }
  }
  set_volume(config_.volume);
}

void GameStack::update(bool controller_change) {
  // Compute input frame.
  auto input = adapter_.ui_frame(controller_change);

  prev_cursor_ = cursor_;
  cursor_ = input.global.mouse_cursor;
  if (input.show_cursor && cursor_frame_ < kCursorFrames) {
    cursor_frame_ = std::min(kCursorFrames, cursor_frame_ + 3u);
  } else if (!input.show_cursor && cursor_frame_) {
    cursor_frame_ -= std::min(cursor_frame_, 2u);
  }
  if (!input.show_cursor) {
    input.global.mouse_cursor.reset();
    for (auto& frame : input.assignments) {
      frame.mouse_cursor.reset();
    }
  }
  bool assignment_mouse_active = false;
  for (auto& frame : input.assignments) {
    if (frame.mouse_cursor) {
      assignment_mouse_active = frame.mouse_active = true;
    }
  }
  input.global.mouse_active = !assignment_mouse_active && input.global.mouse_cursor.has_value();

  std::erase_if(layers_, [](const auto& e) { return e->is_removed(); });
  io_layer_.capture_mouse(!layers_.empty() && +(top()->layer_flags() & layer_flag::kCaptureCursor));
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureUpdate);
  auto input_it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureInput);
  auto get_target = [&](const GameLayer& layer) {
    return render::target{.screen_dimensions = io_layer_.dimensions(),
                          .render_dimensions = layer.bounds().size};
  };

  auto size = layers_.size();
  for (; it != layers_.end(); ++it) {
    auto& e = *it;
    multi_input_frame layer_input;
    if (it >= input_it) {
      auto target = get_target(**it);
      layer_input = input;
      auto map_cursor = [&](input_frame& frame) {
        if (frame.mouse_cursor) {
          auto cursor = *frame.mouse_cursor;
          frame.mouse_cursor = target.screen_to_irender_coords(cursor);
          frame.mouse_delta =
              *frame.mouse_cursor - target.screen_to_irender_coords(cursor - *frame.mouse_delta);
        } else {
          frame.mouse_delta.reset();
        }
      };
      map_cursor(layer_input.global);
      std::for_each(layer_input.assignments.begin(), layer_input.assignments.end(), map_cursor);
      // TODO: avoiding double-inputs at moment UI changes, but likely need same inside elements?
      if (std::distance(layers_.begin(), it) >= static_cast<std::ptrdiff_t>(size)) {
        layer_input.global.join_game_inputs.clear();
        layer_input.global.key_pressed.fill(false);
        for (auto& frame : layer_input.assignments) {
          frame.join_game_inputs.clear();
          frame.key_pressed.fill(false);
        }
      }
    }
    output_frame output;
    if (it >= input_it) {
      e->update_focus(layer_input, output);
    }
    e->update(layer_input, output);
    for (auto s : output.sounds) {
      play_sound(s);
    }
  }

  // Auto-focus new menus for non-mouse inputs.
  if (size != layers_.size() && !empty() && !top()->has_focus() && !input.show_cursor &&
      !(top()->layer_flags() & layer_flag::kNoAutoFocus)) {
    top()->focus(/* last */ false);
  }
  ++cursor_anim_frame_;
}

void GameStack::render(render::GlRenderer& renderer) const {
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureRender);
  for (; it != layers_.end(); ++it) {
    renderer.target().render_dimensions = static_cast<uvec2>((*it)->bounds().size);
    (*it)->render(renderer);
    renderer.clear_depth();
  }
  if (cursor_ && cursor_frame_ &&
      (layers_.empty() || !(top()->layer_flags() & layer_flag::kCaptureCursor))) {
    // TODO: probably replace this once we can render shape fills with cool effects!
    auto hue = cursor_hue_.value_or(1.f / 3);
    renderer.target().render_dimensions = {640, 360};
    auto scale = static_cast<float>(cursor_frame_) / kCursorFrames;
    scale = 1.f / 16 + (15.f / 16) * (1.f - (1.f - scale * scale));
    auto radius = scale * 8.f;
    auto origin = renderer.target().screen_to_frender_coords(*cursor_) +
        from_polar(glm::pi<float>() / 3.f, radius);
    std::optional<render::motion_trail> trail;
    if (prev_cursor_) {
      trail = render::motion_trail{.prev_origin =
                                       renderer.target().screen_to_frender_coords(*prev_cursor_) +
                                       from_polar(glm::pi<float>() / 3.f, radius)};
    }
    auto flash = (64.f - cursor_anim_frame_ % 64) / 64.f;
    std::vector cursor_shapes = {
        render::shape{
            .origin = origin + fvec2{2.f, 3.f},
            .colour = {0.f, 0.f, 0.f, .25f},
            .z_index = 127.5f,
            .trail = trail
                ? std::make_optional(render::motion_trail{trail->prev_origin + fvec2{2.f, 3.f}})
                : std::nullopt,
            .data = render::ngon{.radius = radius, .sides = 3, .line_width = radius / 2},
        },
        render::shape{
            .origin = origin,
            .colour = {hue, .5f, .5f, 1.f},
            .z_index = 127.6f,
            .data = render::ngon{.radius = radius, .sides = 3, .line_width = radius / 2},
        },
        render::shape{
            .origin = origin,
            .colour = {0.f, 0.f, 0.f, 1.f},
            .z_index = 128.f,
            .trail = trail,
            .data = render::ngon{.radius = radius, .sides = 3, .line_width = 1.5f},
        },
        render::shape{
            .origin = origin,
            .colour = {hue, .5f, .85f, 1.f},
            .z_index = 127.8f,
            .data = render::ngon{.radius = radius * flash,
                                 .sides = 3,
                                 .line_width = std::min(radius * flash / 2.f, 1.5f)},
        },
    };
    renderer.render_shapes(render::coordinate_system::kGlobal, cursor_shapes,
                           render::shape_style::kNone);
  }
}

void GameStack::write_config() {
  auto data = data::write_config(config_);
  if (data) {
    // TODO: error reporting (and below).
    (void)fs_.write_config(*data);
  }
}

void GameStack::write_savegame() {
  auto data = data::write_savegame(save_);
  if (data) {
    (void)fs_.write_savegame(kSaveName, *data);
  }
}

void GameStack::write_replay(const data::ReplayWriter& writer, const std::string& name,
                             std::uint64_t score) {
  std::stringstream ss;
  auto mode = writer.initial_conditions().mode;
  ss << writer.initial_conditions().seed << "_" << writer.initial_conditions().player_count << "p_"
     << (mode == game_mode::kLegacy_Boss       ? "bossmode_"
             : mode == game_mode::kLegacy_Hard ? "hardmode_"
             : mode == game_mode::kLegacy_Fast ? "fastmode_"
             : mode == game_mode::kLegacy_What ? "w-hatmode_"
                                               : "")
     << name << "_" << score;

  auto data = writer.write();
  if (data) {
    (void)fs_.write_replay(ss.str(), *data);
  }
}

void GameStack::set_volume(float volume) {
  mixer_.set_master_volume(volume);
}

void GameStack::play_sound(sound s) {
  play_sound(s, 1.f, 0.f, 1.f);
}

void GameStack::play_sound(sound s, float volume, float pan, float pitch) {
  mixer_.play(static_cast<Mixer::audio_handle_t>(s), volume, pan, pitch);
}

}  // namespace ii::ui
