#ifndef II_GAME_CORE_TOOLKIT_PANEL_H
#define II_GAME_CORE_TOOLKIT_PANEL_H
#include "game/common/math.h"
#include "game/core/ui/element.h"
#include "game/render/data/panel.h"

namespace ii::ui {

class Panel : public Element {
public:
  using Element::Element;

  Panel& set_style(render::panel_style style) {
    style_ = style;
    return *this;
  }

  Panel& set_colour(const cvec4& colour) {
    colour_ = colour;
    return *this;
  }

  Panel& set_border(const cvec4& border) {
    border_ = border;
    return *this;
  }

  Panel& set_padding(const ivec2& padding) {
    padding_ = padding;
    return *this;
  }

  Panel& set_margin(const ivec2& margin) {
    margin_min_ = margin_max_ = margin;
    return *this;
  }

  Panel& set_margin(const ivec2& margin_min, const ivec2& margin_max) {
    margin_min_ = margin_min;
    margin_max_ = margin_max;
    return *this;
  }

protected:
  void update_content(const input_frame&, output_frame&) override;
  void render_content(render::GlRenderer&) const override;

private:
  render::panel_style style_ = render::panel_style::kNone;
  cvec4 colour_{1.f};
  cvec4 border_{0.f};
  ivec2 padding_{0};
  ivec2 margin_min_{0};
  ivec2 margin_max_{0};
};

}  // namespace ii::ui

#endif
