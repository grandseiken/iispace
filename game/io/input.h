#ifndef IISPACE_GAME_IO_INPUT_H
#define IISPACE_GAME_IO_INPUT_H
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

const char* to_string(axis a, type t);
const char* to_string(button b, type t);

struct info {
  controller::type type = controller::type::kOther;
  std::string name;
  std::optional<std::uint32_t> player_index;
};

struct frame {
  struct button_event {
    bool down = false;
    controller::button button = controller::button::kMax;
  };

  std::vector<button_event> button_events;
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

enum class key {
  k0,
  k1,
  k2,
  k3,
  k4,
  k5,
  k6,
  k7,
  k8,
  k9,
  kA,
  kB,
  kC,
  kD,
  kE,
  kF,
  kG,
  kH,
  kI,
  kJ,
  kK,
  kL,
  kM,
  kN,
  kO,
  kP,
  kQ,
  kR,
  kS,
  kT,
  kU,
  kV,
  kW,
  kX,
  kY,
  kZ,
  kF1,
  kF2,
  kF3,
  kF4,
  kF5,
  kF6,
  kF7,
  kF8,
  kF9,
  kF10,
  kF11,
  kF12,
  kF13,
  kF14,
  kF15,
  kF16,
  kF17,
  kF18,
  kF19,
  kF20,
  kF21,
  kF22,
  kF23,
  kF24,
  kEscape,
  kTab,
  kCapslock,
  kBackspace,
  kSpacebar,
  kReturn,
  kLShift,
  kRShift,
  kLAlt,
  kRAlt,
  kLCtrl,
  kRCtrl,
  kLSuper,
  kRSuper,
  kLArrow,
  kRArrow,
  kUArrow,
  kDArrow,
  kGrave,
  kBackslash,
  kComma,
  kQuote,
  kSlash,
  kMinus,
  kSemicolon,
  kPeriod,
  kLBracket,
  kRBracket,
  kLParen,
  kRParen,
  kEquals,
  kLess,
  kGreater,
  kAmpersand,
  kAsterisk,
  kAt,
  kCaret,
  kColon,
  kDollar,
  kHash,
  kExclamation,
  kPercent,
  kPlus,
  kQuestion,
  kDoubleQuote,
  kUnderscore,
  kHome,
  kEnd,
  kDelete,
  kInsert,
  kPageDown,
  kPageUp,
  kPrintScreen,
  kPauseBreak,
  kScrollLock,
  kNumlock,
  kKeypad0,
  kKeypad1,
  kKeypad2,
  kKeypad3,
  kKeypad4,
  kKeypad5,
  kKeypad6,
  kKeypad7,
  kKeypad8,
  kKeypad9,
  kKeypadEnter,
  kKeypadPeriod,
  kKeypadPlus,
  kKeypadMinus,
  kKeypadMultiply,
  kKeypadDivide,
  kMax,
};

const char* to_string(key k);

struct frame {
  struct key_event {
    bool down = false;
    keyboard::key key = keyboard::key::kMax;
  };

  std::vector<key_event> key_events;
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_state = {false};
  // TODO: std::string char_delta?

  bool key(keyboard::key k) const {
    return key_state[static_cast<std::size_t>(k)];
  }

  bool& key(keyboard::key k) {
    return key_state[static_cast<std::size_t>(k)];
  }
};

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

const char* to_string(button);

struct frame {
  struct button_event {
    bool down = false;
    mouse::button button = mouse::button::kMax;
  };

  std::vector<button_event> button_events;
  std::array<bool, static_cast<std::size_t>(button::kMax)> button_state = {false};
  glm::ivec2 cursor = {0, 0};
  glm::ivec2 cursor_delta = {0, 0};
  glm::ivec2 wheel_delta = {0, 0};

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