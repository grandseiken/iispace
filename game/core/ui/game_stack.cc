#include "game/core/ui/game_stack.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <array>
#include <span>

namespace ii::ui {
namespace {
const char* kSaveName = "iispace";

template <typename T>
bool contains(std::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

std::span<const io::keyboard::key> keys_for(key k) {
  using type = io::keyboard::key;
  static constexpr std::array accept = {type::kReturn, type::kSpacebar};
  static constexpr std::array cancel = {type::kEscape};
  static constexpr std::array escape = {type::kEscape};
  static constexpr std::array up = {type::kW, type::kUArrow};
  static constexpr std::array down = {type::kS, type::kDArrow};
  static constexpr std::array left = {type::kA, type::kLArrow};
  static constexpr std::array right = {type::kD, type::kRArrow};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kStart:
    return {};
  case key::kEscape:
    return escape;
  case key::kUp:
    return up;
  case key::kDown:
    return down;
  case key::kLeft:
    return left;
  case key::kRight:
    return right;
  default:
    return {};
  }
}

std::span<const io::mouse::button> mouse_buttons_for(key k) {
  using type = io::mouse::button;
  static constexpr std::array accept = {type::kL};
  static constexpr std::array cancel = {type::kR};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  default:
    return {};
  }
}

std::span<const io::controller::button> controller_buttons_for(key k) {
  using type = io::controller::button;
  static constexpr std::array accept = {type::kA};
  static constexpr std::array cancel = {type::kB};
  static constexpr std::array start = {type::kStart, type::kGuide};
  static constexpr std::array up = {type::kDpadUp};
  static constexpr std::array down = {type::kDpadDown};
  static constexpr std::array left = {type::kDpadLeft};
  static constexpr std::array right = {type::kDpadRight};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kStart:
    return start;
  case key::kEscape:
    return {};
  case key::kUp:
    return up;
  case key::kDown:
    return down;
  case key::kLeft:
    return left;
  case key::kRight:
    return right;
  default:
    return {};
  }
}

void handle(input_frame& result, const io::mouse::frame& frame) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto buttons = mouse_buttons_for(k);
    for (auto b : buttons) {
      result.held(k) |= frame.button(b);
    }
    for (const auto& e : frame.button_events) {
      if (e.down && contains(buttons, e.button)) {
        result.pressed(k) = true;
      }
    }
  }
  result.mouse_delta = frame.cursor_delta;
  result.mouse_cursor = frame.cursor;
  result.mouse_scroll = frame.wheel_delta;
}

void handle(input_frame& result, const io::keyboard::frame& frame) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto keys = keys_for(k);
    for (auto kbk : keys) {
      result.held(k) |= frame.key(kbk);
    }
    for (const auto& e : frame.key_events) {
      if (e.down && contains(keys, e.key)) {
        result.pressed(k) = true;
      }
    }
  }
}

void handle(input_frame& result, const io::controller::frame& frame) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto buttons = controller_buttons_for(k);
    for (auto b : buttons) {
      result.held(k) |= frame.button(b);
    }
    for (const auto& e : frame.button_events) {
      if (e.down && contains(buttons, e.button)) {
        result.pressed(k) = true;
      }
    }
  }
}

template <typename It>
auto get_capture_it(It begin, It end, layer_flag flag) {
  auto it = end;
  while (it != begin) {
    --it;
    if (+((*it)->flags() & flag)) {
      break;
    }
  }
  return it;
}

}  // namespace

void GameLayer::update_content(const input_frame&, ui::output_frame&) {
  for (auto& e : *this) {
    e->set_bounds(bounds());
  }
}

GameStack::GameStack(io::Filesystem& fs, io::IoLayer& io_layer, Mixer& mixer)
: fs_{fs}, io_layer_{io_layer}, mixer_{mixer} {
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
  input_frame input;
  input.controller_change = controller_change;
  handle(input, io_layer_.keyboard_frame());
  handle(input, io_layer_.mouse_frame());
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    handle(input, io_layer_.controller_frame(i));
  }
  cursor_ = input.mouse_cursor;
  io_layer_.capture_mouse(!layers_.empty() && +(top()->flags() & layer_flag::kCaptureCursor));
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureUpdate);
  auto input_it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureInput);
  while (it != layers_.end()) {
    auto& e = *it;
    input_frame layer_input;
    if (it >= input_it) {
      render::target target{.screen_dimensions = io_layer_.dimensions(),
                            .render_dimensions = (*it)->bounds().size};
      layer_input = input;
      if (input.mouse_cursor) {
        layer_input.mouse_cursor = target.screen_to_render_coords(*input.mouse_cursor);
        layer_input.mouse_delta = *layer_input.mouse_cursor -
            target.screen_to_render_coords(*input.mouse_cursor - *input.mouse_delta);
      } else {
        layer_input.mouse_delta.reset();
      }
    }
    auto size = layers_.size();
    output_frame output;
    e->update(layer_input, output);
    if (it >= input_it) {
      if (!e->has_focus()) {
        if (layer_input.pressed(ui::key::kUp) || input.pressed(ui::key::kLeft)) {
          e->focus(/* last */ true);
        } else if (layer_input.pressed(ui::key::kDown) || input.pressed(ui::key::kRight)) {
          e->focus(/* last */ false);
        }
        if (e->has_focus()) {
          play_sound(sound::kMenuClick);
          layer_input.key_pressed.fill(false);
        }
      }
      e->update_focus(layer_input, output);
    }
    for (auto s : output.sounds) {
      play_sound(s);
    }
    if (size != layers_.size()) {
      input = {};
    }
    if (e->is_removed()) {
      it = layers_.erase(it);
    } else {
      ++it;
    }
  }
}

void GameStack::render(render::GlRenderer& renderer) const {
  renderer.target().screen_dimensions = io_layer_.dimensions();
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureRender);
  for (; it != layers_.end(); ++it) {
    renderer.target().render_dimensions = static_cast<glm::uvec2>((*it)->bounds().size);
    (*it)->render(renderer);
  }
  renderer.target().render_dimensions = {640, 360};
  if (cursor_ && (layers_.empty() || !(top()->flags() & layer_flag::kCaptureCursor))) {
    // Render cursor.
    auto offset = static_cast<glm::ivec2>(glm::round(from_polar(glm::pi<float>() / 3.f, 8.f)));
    render::shape cursor_shape{
        .origin = offset + renderer.target().screen_to_render_coords(*cursor_),
        .colour = colour_hue360(120),
        .z_index = 128.f,
        .data = render::ngon{.radius = 8.f, .sides = 3, .line_width = 2.f},
    };
    renderer.render_shapes(render::coordinate_system::kGlobal, {&cursor_shape, 1}, 0.f);
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
     << (mode == game_mode::kBoss       ? "bossmode_"
             : mode == game_mode::kHard ? "hardmode_"
             : mode == game_mode::kFast ? "fastmode_"
             : mode == game_mode::kWhat ? "w-hatmode_"
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