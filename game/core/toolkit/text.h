#ifndef II_GAME_CORE_TOOLKIT_TEXT_H
#define II_GAME_CORE_TOOLKIT_TEXT_H
#include "game/common/ustring.h"
#include "game/core/ui/element.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ii::ui {

class TextElement : public Element {
public:
  using Element::Element;

  TextElement& set_font(render::font_id font) {
    if (font_ != font) {
      font_ = font;
      dirty_ = true;
    }
    return *this;
  }

  TextElement& set_font_dimensions(const glm::uvec2& dimensions) {
    if (font_dimensions_ != dimensions) {
      font_dimensions_ = dimensions;
      dirty_ = true;
    }
    return *this;
  }

  TextElement& set_colour(const glm::vec4& colour) {
    colour_ = colour;
    return *this;
  }

  TextElement& set_drop_shadow(const glm::ivec2& offset, float opacity) {
    drop_shadow_ = drop_shadow{offset, opacity};
    return *this;
  }

  TextElement& set_multiline(bool multiline) {
    if (multiline_ != multiline) {
      multiline_ = multiline;
      dirty_ = true;
    }
    return *this;
  }

  TextElement& set_text(ustring&& text) {
    text_ = std::move(text);
    dirty_ = true;
    return *this;
  }

protected:
  void render_content(render::GlRenderer&) const override;

private:
  struct drop_shadow {
    glm::ivec2 offset{0};
    float opacity = 0.f;
  };

  render::font_id font_ = render::font_id::kDefault;
  glm::uvec2 font_dimensions_{0};
  glm::vec4 colour_{1.f};
  std::optional<drop_shadow> drop_shadow_;
  bool multiline_ = false;
  ustring text_ = ustring::ascii("");

  mutable rect cached_bounds_;
  mutable glm::uvec2 render_dimensions_{0};
  mutable std::vector<std::u32string> lines_;
  mutable bool dirty_ = false;
};

}  // namespace ii::ui

#endif
