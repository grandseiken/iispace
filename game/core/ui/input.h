#ifndef II_GAME_CORE_UI_INPUT_H
#define II_GAME_CORE_UI_INPUT_H
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <optional>
#include <unordered_set>
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

struct input_device_id {
  // TODO: replace this with a stable reference. Needs IoLayer support.
  std::optional<std::size_t> controller_index;

  static input_device_id kbm() { return {}; }
  static input_device_id controller(std::size_t index) { return {index}; }
};

struct input_frame {
  bool controller_change = false;
  bool pad_navigation = false;
  bool mouse_active = false;
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_pressed = {false};
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_held = {false};
  std::optional<glm::ivec2> mouse_delta;
  std::optional<glm::ivec2> mouse_cursor;
  std::optional<glm::ivec2> mouse_scroll;
  std::optional<glm::ivec2> controller_scroll;
  std::vector<input_device_id> join_game_inputs;

  bool pressed(key k) const { return key_pressed[static_cast<std::size_t>(k)]; }
  bool& pressed(key k) { return key_pressed[static_cast<std::size_t>(k)]; }

  bool held(key k) const { return key_held[static_cast<std::size_t>(k)]; }
  bool& held(key k) { return key_held[static_cast<std::size_t>(k)]; }
};

struct multi_input_frame {
  bool show_cursor = false;
  input_frame global;
  std::vector<input_frame> assignments;
};

struct output_frame {
  // TODO: dedupe/mix this like with aggregates...
  std::unordered_set<sound> sounds;
};

}  // namespace ii::ui

#endif
