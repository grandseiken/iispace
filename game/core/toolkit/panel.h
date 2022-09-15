#ifndef II_GAME_CORE_TOOLKIT_PANEL_H
#define II_GAME_CORE_TOOLKIT_PANEL_H
#include "game/core/ui/element.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>

namespace ii::ui {

class Panel : public Element {
public:
  using Element::Element;

  Panel& set_style(render::panel_style style) {
    style_ = style;
    return *this;
  }

  Panel& set_colour(const glm::vec4& colour) {
    colour_ = colour;
    return *this;
  }

  Panel& set_padding(const glm::ivec2& padding) {
    padding_ = padding;
    return *this;
  }

  Panel& set_margin(const glm::ivec2& margin) {
    margin_ = margin;
    return *this;
  }

protected:
  void update_content(const input_frame&, output_frame&) override;
  void render_content(render::GlRenderer&) const override;

private:
  render::panel_style style_ = render::panel_style::kNone;
  glm::vec4 colour_{1.f};
  glm::ivec2 padding_{0};
  glm::ivec2 margin_{0};
};

}  // namespace ii::ui

#endif
