#include "game/render/gl_renderer.h"
#include "game/render/gl/data.h"
#include "game/render/gl/draw.h"
#include "game/render/gl/program.h"
#include "game/render/gl/texture.h"
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <nonstd/span.hpp>

namespace {
#include "game/render/shaders/rect.f.glsl.h"
#include "game/render/shaders/rect.v.glsl.h"
#include "game/render/shaders/text.f.glsl.h"
#include "game/render/shaders/text.v.glsl.h"

const nonstd::span<const std::uint8_t> kRectFragmentShader = {game_render_shaders_rect_f_glsl,
                                                              game_render_shaders_rect_f_glsl_len};
const nonstd::span<const std::uint8_t> kRectVertexShader = {game_render_shaders_rect_v_glsl,
                                                            game_render_shaders_rect_v_glsl_len};
const nonstd::span<const std::uint8_t> kTextFragmentShader = {game_render_shaders_text_f_glsl,
                                                              game_render_shaders_text_f_glsl_len};
const nonstd::span<const std::uint8_t> kTetVertexShader = {game_render_shaders_text_v_glsl,
                                                           game_render_shaders_text_v_glsl_len};
}  // namespace

namespace ii {

struct GlRenderer::impl_t {};

result<GlRenderer> GlRenderer::create() {
  GlRenderer renderer{{}};
  renderer.impl_ = std::make_unique<impl_t>();
  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
GlRenderer::GlRenderer(GlRenderer&&) = default;
GlRenderer& GlRenderer::operator=(GlRenderer&&) = default;
GlRenderer::~GlRenderer() = default;

}  // namespace ii