#include "game/core/ui_layer.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include <nonstd/span.hpp>

namespace ii::ui {
namespace {
const std::string kSaveName = "iispace";

template <typename T>
bool contains(nonstd::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

nonstd::span<const io::keyboard::key> keys_for(key k) {
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

nonstd::span<const io::mouse::button> mouse_buttons_for(key k) {
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

nonstd::span<const io::controller::button> controller_buttons_for(key k) {
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

UiLayer::UiLayer(io::Filesystem& fs, io::IoLayer& io_layer, Mixer& mixer)
: fs_{fs}, io_layer_{io_layer}, mixer_{mixer} {
  auto data = fs.read_config();
  if (data) {
    auto config = ii::Config::load(*data);
    if (config) {
      config_ = std::move(*config);
    }
  }
  data = fs.read_savegame(kSaveName);
  if (data) {
    auto save_data = ii::SaveGame::load(*data);
    if (save_data) {
      save_ = std::move(*save_data);
    }
  }
  set_volume(config_.volume);
}

void UiLayer::compute_input_frame(bool controller_change) {
  input_ = {};
  input_.controller_change = controller_change;
  handle(input_, io_layer_.keyboard_frame());
  handle(input_, io_layer_.mouse_frame());
  for (std::size_t i = 0; i < io_layer_.controllers(); ++i) {
    handle(input_, io_layer_.controller_frame(i));
  }
}

void UiLayer::write_config() {
  auto data = config_.save();
  if (data) {
    // TODO: error reporting (and below).
    (void)fs_.write_config(*data);
  }
}

void UiLayer::write_save_game() {
  auto data = save_.save();
  if (data) {
    (void)fs_.write_savegame(kSaveName, *data);
  }
}

void UiLayer::write_replay(const ii::ReplayWriter& writer, const std::string& name,
                           std::int64_t score) {
  std::stringstream ss;
  auto mode = writer.initial_conditions().mode;
  ss << writer.initial_conditions().seed << "_" << writer.initial_conditions().player_count << "p_"
     << (mode == ii::game_mode::kBoss       ? "bossmode_"
             : mode == ii::game_mode::kHard ? "hardmode_"
             : mode == ii::game_mode::kFast ? "fastmode_"
             : mode == ii::game_mode::kWhat ? "w-hatmode_"
                                            : "")
     << name << "_" << score;

  auto data = writer.write();
  if (data) {
    (void)fs_.write_replay(ss.str(), *data);
  }
}

void UiLayer::rumble(std::int32_t time) {
  // TODO
}

void UiLayer::set_volume(float volume) {
  mixer_.set_master_volume(volume / 100.f);
}

void UiLayer::play_sound(sound s) {
  play_sound(s, 1.f, 0.f, 1.f);
}

void UiLayer::play_sound(sound s, float volume, float pan, float pitch) {
  mixer_.play(static_cast<ii::Mixer::audio_handle_t>(s), volume, pan, pitch);
}

}  // namespace ii::ui