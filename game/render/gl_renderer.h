#ifndef II_GAME_RENDER_GL_RENDERER_H
#define II_GAME_RENDER_GL_RENDERER_H
#include "game/common/rect.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
#include "game/render/data/background.h"
#include "game/render/data/panel.h"
#include "game/render/data/shapes.h"
#include "game/render/data/text.h"
#include "game/render/target.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace ii::render {

class GlRenderer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<GlRenderer>> create(std::uint32_t shader_version);
  GlRenderer(access_tag);
  GlRenderer(GlRenderer&&) = delete;
  GlRenderer& operator=(GlRenderer&&) = delete;
  ~GlRenderer();

  result<void> status() const;
  void clear_screen() const;
  void clear_depth() const;

  render::target& target() { return target_; }
  const render::target& target() const { return target_; }

  // TODO: eliminate these functions?
  void set_colour_cycle(std::uint32_t cycle) { colour_cycle_ = cycle; }
  std::uint32_t colour_cycle() const { return colour_cycle_; }

  // TODO: rework font support:
  // - support rendering with multiple fonts, so we can do multilingual text with e.g. Noto Sans.
  // - allow rendering multiple texts (with same font?), for example for multiline TextElement.
  // - below methods should be extracted from renderer somehow, maybe moved into FontCache.
  std::int32_t line_height(const font_data& font) const;
  std::int32_t text_width(const font_data& font, ustring_view s) const;
  ustring trim_for_width(const font_data& font, std::int32_t width, ustring_view s) const;
  // TODO: oklab rendering has certainly fucked lcd text-blending, need to fix.
  void render_text(const font_data& font, const glm::ivec2& position, const glm::vec4& colour,
                   bool clip, ustring_view s) const;
  void render_text(const font_data& font, const rect& bounds, alignment align,
                   const glm::vec4& colour, bool clip, const std::vector<ustring>& lines) const;

  // TODO: render multiple panels?
  // TODO: in general we can maybe render UI with minimal draw calls via a breadth-first search
  // that renders everything at element depth N (collapsing elements that render nothing) in a
  // single pass, etc.
  void render_panel(const panel_data&) const;
  void render_panel(const combo_panel&) const;

  void render_background(const render::background& data) const;
  // TODO: _maybe_ render outlines automatically somehow, or output from geometry shapes?
  // TODO: 3D shadows; lighting explosion effects.
  void render_shapes(coordinate_system ctype, std::vector<render::shape>& shapes,
                     shape_style style) const;
  void render_present() const;

private:
  render::target target_;
  std::uint32_t colour_cycle_ = 0;
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

std::vector<ustring> prepare_text(const render::GlRenderer& r, const render::font_data& font,
                                  bool multiline, std::int32_t bounds_width, ustring_view s);

}  // namespace ii::render

#endif