#ifndef II_GAME_CORE_UI_INPUT_H
#define II_GAME_CORE_UI_INPUT_H
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace ii::ui {

enum class key {
  kClick,
  kAccept,
  kCancel,
  kStart,
  kEscape,
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
  std::optional<glm::ivec2> mouse_delta;
  std::optional<glm::ivec2> mouse_cursor;
  std::optional<glm::ivec2> mouse_scroll;
  bool pad_navigation = false;

  bool pressed(key k) const { return key_pressed[static_cast<std::size_t>(k)]; }
  bool& pressed(key k) { return key_pressed[static_cast<std::size_t>(k)]; }

  bool held(key k) const { return key_held[static_cast<std::size_t>(k)]; }
  bool& held(key k) { return key_held[static_cast<std::size_t>(k)]; }
};

struct output_frame {
  std::vector<sound> sounds;
};

}  // namespace ii::ui

#endif