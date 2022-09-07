#ifndef II_GAME_RENDER_GL_RENDERER_H
#define II_GAME_RENDER_GL_RENDERER_H
#include "game/common/rect.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
#include "game/logic/sim/io/render.h"
#include "game/render/render_common.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

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

  render::target& target() { return target_; }
  const render::target& target() const { return target_; }

  // TODO: eliminate these functions?
  void set_colour_cycle(std::uint32_t cycle) { colour_cycle_ = cycle; }
  std::uint32_t colour_cycle() const { return colour_cycle_; }

  // TODO: these methods should be extracted from renderer somehow, maybe moved into FontCache.
  std::int32_t line_height(font_id font, const glm::uvec2& font_dimensions) const;
  std::int32_t text_width(font_id font, const glm::uvec2& font_dimensions, ustring_view s) const;
  ustring trim_for_width(font_id font, const glm::uvec2& font_dimensions, std::int32_t width,
                         ustring_view s) const;
  // TODO: render multiple texts (with same font)?
  void render_text(font_id font, const glm::uvec2& font_dimensions, const glm::ivec2& position,
                   const glm::vec4& colour, ustring_view s) const;

  // TODO: render multiple panels?
  void render_panel(const panel_data&);

  void
  render_shapes(coordinate_system ctype, std::span<const shape> shapes, float trail_alpha) const;

private:
  render::target target_;
  std::uint32_t colour_cycle_ = 0;
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::render

#endif