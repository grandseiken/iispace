#ifndef II_GAME_RENDER_DATA_TEXT_H
#define II_GAME_RENDER_DATA_TEXT_H
#include "game/common/enum.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace ii::render {
enum class alignment : std::uint32_t {
  kCentered = 0b0000,
  kLeft = 0b0001,
  kRight = 0b0010,
  kTop = 0b0100,
  kBottom = 0b1000,
};

enum class font_id : std::uint32_t {
  kDefault,
  kMonospace = kDefault,
  kMonospaceBold,
  kMonospaceItalic,
  kMonospaceBoldItalic,
};

struct font_data {
  font_id id = font_id::kDefault;
  glm::uvec2 dimensions{0u};
};

}  // namespace ii::render

namespace ii {
template <>
struct bitmask_enum<render::alignment> : std::true_type {};
}  // namespace ii

#endif