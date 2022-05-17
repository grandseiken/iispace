#ifndef IISPACE_GAME_IO_IO_H
#define IISPACE_GAME_IO_IO_H
#include "game/io/input.h"
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