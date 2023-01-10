#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/position_fragment.glsl"

flat in uint g_style;
flat in vec4 g_colour;
flat in vec2 g_position;
flat in vec2 g_dimensions;
flat in vec2 g_seed;

out vec4 out_colour;

void main() {
  vec2 frag_position = game_position(gl_FragCoord);
  float d_sq = dot(frag_position - g_position, frag_position - g_position);

  switch (g_style) {
  case kFxStyleExplosion:
    if (g_dimensions.y * g_dimensions.y <= d_sq && d_sq <= g_dimensions.x * g_dimensions.x) {
      out_colour = g_colour;
    } else {
      discard;
    }
    break;

  default:
    out_colour = g_colour;
    break;
  }
}