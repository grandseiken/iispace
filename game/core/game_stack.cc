#include "game/core/game_stack.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include <algorithm>
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
  static constexpr type accept[] = {type::kZ, type::kSpacebar, type::kLCtrl, type::kRCtrl};
  static constexpr type cancel[] = {type::kX, type::kEscape};
  static constexpr type menu[] = {type::kEscape, type::kReturn};
  static constexpr type up[] = {type::kW, type::kUArrow};
  static constexpr type down[] = {type::kS, type::kDArrow};
  static constexpr type left[] = {type::kA, type::kLArrow};
  static constexpr type right[] = {type::kD, type::kRArrow};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kMenu:
    return menu;
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
  static constexpr type accept[] = {type::kL};
  static constexpr type cancel[] = {type::kR};

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
  static constexpr type accept[] = {type::kA, type::kRShoulder};
  static constexpr type cancel[] = {type::kB, type::kLShoulder};
  static constexpr type menu[] = {type::kStart, type::kGuide};
  static constexpr type up[] = {type::kDpadUp};
  static constexpr type down[] = {type::kDpadDown};
  static constexpr type left[] = {type::kDpadLeft};
  static constexpr type right[] = {type::kDpadRight};

  switch (k) {
  case key::kAccept:
    return accept;
  case key::kCancel:
    return cancel;
  case key::kMenu:
    return menu;
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

}  // namespace

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
  auto it = layers_.end();
  while (it != layers_.begin()) {
    --it;
    if (+((*it)->flags() & layer_flag::kCaptureUpdate)) {
      break;
    }
  }
  while (it != layers_.end()) {
    auto size = layers_.size();
    (*it)->update(input);
    if (size != layers_.size()) {
      input = {};
    }
    if ((*it)->is_closed()) {
      it = layers_.erase(it);
    } else {
      ++it;
    }
  }
}

void GameStack::render(render::GlRenderer& renderer) const {
  auto it = layers_.end();
  while (it != layers_.begin()) {
    --it;
    if (+((*it)->flags() & layer_flag::kCaptureRender)) {
      break;
    }
  }
  for (; it != layers_.end(); ++it) {
    (*it)->render(renderer);
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
  mixer_.set_master_volume(volume / 100.f);
}

void GameStack::play_sound(sound s) {
  play_sound(s, 1.f, 0.f, 1.f);
}

void GameStack::play_sound(sound s, float volume, float pan, float pitch) {
  mixer_.play(static_cast<Mixer::audio_handle_t>(s), volume, pan, pitch);
}

}  // namespace ii::ui