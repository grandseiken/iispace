#include "game/core/ui/game_stack.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>

namespace ii::ui {
namespace {
const char* kSaveName = "space";
constexpr std::uint32_t kCursorFrames = 32u;
constexpr std::uint32_t kKeyRepeatDelayFrames = 20u;
constexpr std::uint32_t kKeyRepeatIntervalFrames = 5u;

template <typename T>
bool contains(std::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

bool is_navigation(key k) {
  return k == key::kUp || k == key::kDown || k == key::kLeft || k == key::kRight;
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
  static constexpr std::array click = {type::kL};
  static constexpr std::array cancel = {type::kR};

  switch (k) {
  case key::kClick:
    return click;
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
        result.pad_navigation |= is_navigation(k);
      }
    }
  }
}

void handle(input_frame& result, const io::controller::frame& frame, glm::ivec2& previous) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(key::kMax); ++i) {
    auto k = static_cast<key>(i);
    auto buttons = controller_buttons_for(k);
    for (auto b : buttons) {
      result.held(k) |= frame.button(b);
    }
    for (const auto& e : frame.button_events) {
      if (e.down && contains(buttons, e.button)) {
        result.pressed(k) = true;
        result.pad_navigation |= is_navigation(k);
      }
    }
  }

  auto convert_axis = [](std::int16_t v) -> std::int32_t {
    auto f = std::abs(static_cast<float>(v)) / std::numeric_limits<std::int16_t>::max();
    f = std::round(4.f * std::clamp(2.f * f - 1.f, 0.f, 1.f));
    auto i = static_cast<std::int32_t>(f);
    return v >= 0 ? i : -i;
  };

  if (auto x = convert_axis(frame.axis(io::controller::axis::kRX)); x) {
    if (!result.controller_scroll) {
      result.controller_scroll = {0, 0};
    }
    result.controller_scroll->x += x;
  }

  if (auto y = convert_axis(frame.axis(io::controller::axis::kRY)); y) {
    if (!result.controller_scroll) {
      result.controller_scroll = {0, 0};
    }
    result.controller_scroll->y += y;
  }

  auto x = convert_axis(frame.axis(io::controller::axis::kLX));
  auto y = convert_axis(frame.axis(io::controller::axis::kLY));
  result.pad_navigation |= abs(x) > 1 || abs(y) > 1;
  result.held(ui::key::kLeft) |= x < -1;
  result.held(ui::key::kRight) |= x > 1;
  result.held(ui::key::kUp) |= y < -1;
  result.held(ui::key::kDown) |= y > 1;
  result.pressed(ui::key::kLeft) |= x < -1 && previous.x >= -1;
  result.pressed(ui::key::kRight) |= x > 1 && previous.x <= 1;
  result.pressed(ui::key::kUp) |= y < -1 && previous.y >= -1;
  result.pressed(ui::key::kDown) |= y > 1 && previous.y <= 1;
  previous.x = x;
  previous.y = y;
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
  std::erase_if(layers_, [](const auto& e) { return e->is_removed(); });

  // Compute input frame.
  input_frame input;
  input.controller_change = controller_change;
  if (controller_change) {
    prev_controller_.clear();
    for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
      prev_controller_.emplace_back(glm::ivec2{0});
    }
  }
  handle(input, io_layer_.keyboard_frame());
  handle(input, io_layer_.mouse_frame());
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    handle(input, io_layer_.controller_frame(i), prev_controller_[i]);
  }

  for (std::size_t i = 0; i < static_cast<std::size_t>(ui::key::kMax); ++i) {
    if (input.key_held[i] && is_navigation(static_cast<ui::key>(i))) {
      ++key_held_frames[i];
    } else {
      key_held_frames[i] = 0;
    }
    if (key_held_frames[i] >= kKeyRepeatDelayFrames &&
        !((key_held_frames[i] - kKeyRepeatDelayFrames) % kKeyRepeatIntervalFrames)) {
      input.key_pressed[i] = true;
    }
  }

  prev_cursor_ = cursor_;
  cursor_ = input.mouse_cursor;
  if (input.mouse_delta && input.mouse_delta != glm::ivec2{0}) {
    show_cursor_ = true;
  } else if (input.pad_navigation) {
    show_cursor_ = false;
  }
  if (show_cursor_ && cursor_frame_ < kCursorFrames) {
    cursor_frame_ = std::min(kCursorFrames, cursor_frame_ + 3u);
  } else if (!show_cursor_ && cursor_frame_) {
    cursor_frame_ -= std::min(cursor_frame_, 2u);
  }

  io_layer_.capture_mouse(!layers_.empty() && +(top()->flags() & layer_flag::kCaptureCursor));
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureUpdate);
  auto input_it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureInput);
  for (; it != layers_.end(); ++it) {
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
  }
  ++cursor_anim_frame_;
}

void GameStack::render(render::GlRenderer& renderer) const {
  renderer.target().screen_dimensions = io_layer_.dimensions();
  auto it = get_capture_it(layers_.begin(), layers_.end(), layer_flag::kCaptureRender);
  for (; it != layers_.end(); ++it) {
    renderer.target().render_dimensions = static_cast<glm::uvec2>((*it)->bounds().size);
    (*it)->render(renderer);
    renderer.clear_depth();
  }
  if (cursor_ && cursor_frame_ &&
      (layers_.empty() || !(top()->flags() & layer_flag::kCaptureCursor))) {
    // TODO: probably replace this once we can render shape fills with cool effects.
    renderer.target().render_dimensions = {640, 360};
    auto scale = static_cast<float>(cursor_frame_) / kCursorFrames;
    scale = 1.f / 16 + (15.f / 16) * (1.f - (1.f - scale * scale));
    auto radius = scale * 8.f;
    auto origin = static_cast<glm::vec2>(renderer.target().screen_to_render_coords(*cursor_)) +
        from_polar(glm::pi<float>() / 3.f, radius);
    std::optional<render::motion_trail> trail;
    if (prev_cursor_) {
      trail = {.prev_origin = static_cast<glm::vec2>(
                                  renderer.target().screen_to_render_coords(*prev_cursor_)) +
                   from_polar(glm::pi<float>() / 3.f, radius)};
    }
    auto flash = (64.f - cursor_anim_frame_ % 64) / 64.f;
    std::array cursor_shapes = {
        render::shape{
            .origin = origin + glm::vec2{2.f, 2.f},
            .colour = {0.f, 0.f, 0.f, .5f},
            .z_index = 127.5f,
            .trail = trail,
            .data = render::ngon{.radius = radius, .sides = 3, .line_width = radius / 2},
        },
        render::shape{
            .origin = origin,
            .colour = colour_hue360(120, .5f, .5f),
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
            .colour = colour_hue360(120, .85f, .5f),
            .z_index = 127.8f,
            .data = render::ngon{.radius = radius * flash,
                                 .sides = 3,
                                 .line_width = std::min(radius * flash / 2.f, 1.5f)},
        },
    };
    renderer.render_shapes(render::coordinate_system::kGlobal, cursor_shapes, .25f);
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