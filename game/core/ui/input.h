#ifndef II_GAME_CORE_UI_INPUT_H
#define II_GAME_CORE_UI_INPUT_H
#include <array>
#include <cstddef>

namespace ii::ui {

enum class key {
  kAccept,
  kCancel,
  kMenu,
  kUp,
  kDown,
  kLeft,
  kRight,
  kMax,
};

struct input_frame {
  bool controller_change = false;
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_pressed = {false};
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_held = {false};

  bool pressed(key k) const {
    return key_pressed[static_cast<std::size_t>(k)];
  }

  bool& pressed(key k) {
    return key_pressed[static_cast<std::size_t>(k)];
  }

  bool held(key k) const {
    return key_held[static_cast<std::size_t>(k)];
  }

  bool& held(key k) {
    return key_held[static_cast<std::size_t>(k)];
  }
};

}  // namespace ii::ui

#endif