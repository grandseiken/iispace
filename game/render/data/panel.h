#ifndef II_GAME_RENDER_DATA_PANEL_H
#define II_GAME_RENDER_DATA_PANEL_H
#include "game/common/colour.h"
#include "game/common/rect.h"
#include "game/common/ustring.h"
#include "game/render/data/shapes.h"
#include "game/render/data/text.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <variant>
#include <vector>

namespace ii::render {

enum class panel_style : std::uint32_t {
  kNone = 0,
  kFlatColour = 1,
};

struct panel_data {
  panel_style style = panel_style::kNone;
  glm::vec4 colour = colour::kWhite0;
  rect bounds;
};

struct sim_panel {
  struct icon {
    std::vector<shape> shapes;
  };

  struct text {
    font_data font;
    alignment align = alignment::kTop | alignment::kLeft;
    glm::vec4 colour = colour::kWhite0;
    ustring text;
  };

  struct element {
    using variant = std::variant<icon, text>;
    variant e;
    rect bounds;
  };

  panel_data panel;
  std::vector<element> elements;
};

}  // namespace ii::render

#endif