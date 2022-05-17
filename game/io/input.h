#ifndef IISPACE_GAME_IO_INPUT_H
#define IISPACE_GAME_IO_INPUT_H
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace ii::io {
namespace controller {

enum class type {
  kOther,
  kPsx,
  kXbox,
};

enum class axis {
  kLX,
  kLY,
  kRX,
  kRY,
  kLT,
  kRT,
  kMax,
};

enum class button {
  kA,
  kB,
  kX,
  kY,
  kBack,
  kGuide,
  kStart,
  kLStick,
  kRStick,
  kLShoulder,
  kRShoulder,
  kDpadUp,
  kDpadDown,
  kDpadLeft,
  kDpadRight,
  kMisc1,
  kPaddle1,
  kPaddle2,
  kPaddle3,
  kPaddle4,
  kTouchpad,
  kMax,
};

struct info {
  controller::type type = controller::type::kOther;
  std::string name;
  std::optional<std::uint32_t> player_index;
};

struct frame {
  // TODO: button_event.
  std::array<bool, static_cast<std::size_t>(button::kMax)> button_state = {false};
  std::array<std::int16_t, static_cast<std::size_t>(axis::kMax)> axis_state = {0};

  bool button(controller::button k) const {
    return button_state[static_cast<std::size_t>(k)];
  }

  bool& button(controller::button k) {
    return button_state[static_cast<std::size_t>(k)];
  }

  std::int16_t axis(controller::axis k) const {
    return axis_state[static_cast<std::size_t>(k)];
  }

  std::int16_t& axis(controller::axis k) {
    return axis_state[static_cast<std::size_t>(k)];
  }
};

}  // namespace controller

namespace keyboard {
struct frame {};
}  // namespace keyboard

namespace mouse {

enum class button {
  kL,
  kM,
  kR,
  kX1,
  kX2,
  kMax,
};

struct frame {
  // TODO: button_event.
  std::array<bool, static_cast<std::size_t>(button::kMax)> button_state = {false};

  bool button(mouse::button k) const {
    return button_state[static_cast<std::size_t>(k)];
  }

  bool& button(mouse::button k) {
    return button_state[static_cast<std::size_t>(k)];
  }
};

}  // namespace mouse
}  // namespace ii::io

#endif