#ifndef II_GAME_RENDER_DATA_PANEL_H
#define II_GAME_RENDER_DATA_PANEL_H
#include "game/common/rect.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace ii::render {

enum class panel_style : std::uint32_t {
  kNone = 0,
  kFlatColour = 1,
};

struct panel_data {
  panel_style style = panel_style::kNone;
  glm::vec4 colour{1.f};
  rect bounds;
};

}  // namespace ii::render

#endif