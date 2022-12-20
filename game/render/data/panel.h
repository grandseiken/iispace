#ifndef II_GAME_RENDER_DATA_PANEL_H
#define II_GAME_RENDER_DATA_PANEL_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/common/rect.h"
#include "game/common/ustring.h"
#include "game/render/data/shapes.h"
#include "game/render/data/text.h"
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace ii::render {

enum class panel_style : std::uint32_t {
  kNone = 0,
  kFlatColour = 1,
};

struct panel_data {
  panel_style style = panel_style::kNone;
  cvec4 colour = colour::kWhite0;
  cvec4 border = colour::kZero;
  frect bounds;
};

struct combo_panel {
  struct icon {
    std::vector<shape> shapes;
  };

  struct text {
    font_data font;
    alignment align = alignment::kTop | alignment::kLeft;
    cvec4 colour = colour::kWhite0;
    std::optional<render::drop_shadow> drop_shadow;
    ustring text;
    bool multiline = false;
  };

  struct element {
    using variant = std::variant<icon, text>;
    frect bounds;
    variant e;
  };

  panel_data panel;
  ivec2 padding{0};
  std::optional<fvec2> target;
  std::vector<element> elements;
};

}  // namespace ii::render

#endif