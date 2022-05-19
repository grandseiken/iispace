#include "game/render/shaders/shaders.h"

namespace ii::shaders {
namespace {
#include "game/render/shaders/rect.f.glsl.h"
#include "game/render/shaders/rect.v.glsl.h"
#include "game/render/shaders/text.f.glsl.h"
#include "game/render/shaders/text.v.glsl.h"
}  //  namespace

nonstd::span<const std::uint8_t> rect_fragment() {
  return {game_render_shaders_rect_f_glsl, game_render_shaders_rect_f_glsl_len};
}

nonstd::span<const std::uint8_t> rect_vertex() {
  return {game_render_shaders_rect_v_glsl, game_render_shaders_rect_v_glsl_len};
}

nonstd::span<const std::uint8_t> text_fragment() {
  return {game_render_shaders_text_f_glsl, game_render_shaders_text_f_glsl_len};
}

nonstd::span<const std::uint8_t> text_vertex() {
  return {game_render_shaders_text_v_glsl, game_render_shaders_text_v_glsl_len};
}

}  // namespace ii::shaders