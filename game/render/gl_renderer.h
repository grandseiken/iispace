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
#include <string_view>
#include <vector>

namespace ii::render {

enum class shape_style {
  kNone,
  kStandard,
};

// TODO: procedural (simplex) noise. This is tricky.
// - Purely procedural noise (i.e. computed entirely on GPU in fragment shader) is fairly easy and
//   worth experimenting with, but may be too slow e.g. for full-screen fractal (fBm) noise.
// - Purely pregenerated noise may be suitable for small/finite effects, but otherwise texture size
//   gets big fast and seamless scrolling is tricky.
//
// Likely best solution: allow allocating per-use-case noise source objects which store offsets, and
// generate and cache noise textures on the fly. By buffering larger textures than needed, we can
// implement infinitely scrolling noise without having to regenerate too often. For 3D noise we can
// use 2D array textures and interpolate between layers.
//
// Possible libraries that could help:
// - https://github.com/Auburn/FastNoise2
// - https://github.com/Auburn/FastNoiseLite
// - https://github.com/ashima/webgl-noise
// - https://github.com/KdotJPG/OpenSimplex2
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
  std::int32_t line_height(font_id font, const glm::uvec2& font_dimensions) const;
  std::int32_t text_width(font_id font, const glm::uvec2& font_dimensions, ustring_view s) const;
  ustring trim_for_width(font_id font, const glm::uvec2& font_dimensions, std::int32_t width,
                         ustring_view s) const;
  void render_text(font_id font, const glm::uvec2& font_dimensions, const glm::ivec2& position,
                   const glm::vec4& colour, bool clip, ustring_view s) const;

  // TODO: render multiple panels?
  // TODO: in general we can maybe render UI with minimal draw calls via a breadth-first search
  // that renders everything at element depth N (collapsing elements that render nothing) in a
  // single pass, etc.
  void render_panel(const panel_data&);

  void render_background(const glm::vec4& colour);
  // TODO:
  // - _maybe_ render outlines automatically rather than manually?
  //   Or maybe output from geometry shapes?
  void render_shapes(coordinate_system ctype, std::vector<render::shape>& shapes, shape_style style,
                     float trail_alpha) const;

private:
  render::target target_;
  std::uint32_t colour_cycle_ = 0;
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::render

#endif