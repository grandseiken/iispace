#include "game/core/toolkit/text.h"
#include "game/common/ustring_convert.h"
#include "game/render/gl_renderer.h"
#include <cstddef>

namespace ii::ui {

void TextElement::render_content(render::GlRenderer& r) const {
  if (cached_bounds_ != bounds()) {
    cached_bounds_ = bounds();
    dirty_ = true;
  }
  if (render_dimensions_ != r.target().render_dimensions) {
    render_dimensions_ = r.target().render_dimensions;
    dirty_ = true;
  }

  if (dirty_) {
    lines_ = render::prepare_text(r, font_, multiline_, bounds().size.x, text_);
    dirty_ = false;
  }
  if (drop_shadow_) {
    r.render_text(font_, bounds().size_rect() + drop_shadow_->offset, align_,
                  cvec4{0.f, 0.f, 0.f, drop_shadow_->opacity}, /* clip */ false, lines_);
  }
  r.render_text(font_, bounds().size_rect(), align_, colour_, /* clip */ false, lines_);
}

}  // namespace ii::ui