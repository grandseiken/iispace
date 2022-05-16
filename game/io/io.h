#ifndef IISPACE_GAME_IO_IO_H
#define IISPACE_GAME_IO_IO_H
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ii::io {

enum class event_type {
  kClose,
  kControllerChange,
};

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
};

}  // namespace controller

class IoLayer {
public:
  virtual ~IoLayer() = default;

  virtual void swap_buffers() = 0;
  virtual std::optional<event_type> poll() = 0;

  virtual std::vector<controller::info> controller_info() const = 0;
  virtual controller::frame controller_frame(std::size_t index) const = 0;
  virtual void input_frame_clear() = 0;
};

}  // namespace ii::io

#endif