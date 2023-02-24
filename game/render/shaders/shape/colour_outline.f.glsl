#include "game/render/shaders/lib/frag_math.glsl"
#include "game/render/shaders/lib/position_fragment.glsl"
#include "game/render/shaders/shape/data.glsl"

uniform bool is_multisample;

flat in uint g_buffer_index;
centroid in float g_colour_interpolate;
out vec4 out_colour;

layout(std430, binding = 0) restrict readonly buffer shape_buffer_block {
  shape_buffer_data data[];
}
shape_buffer;

layout(std430, binding = 1) restrict readonly buffer ball_buffer_block {
  ball_buffer_data data[];
}
ball_buffer;

void main() {
  shape_buffer_data d = shape_buffer.data[g_buffer_index];
  if (d.style != kShapeStyleBall) {
    out_colour = mix(d.colour0, d.colour1, g_colour_interpolate);
    return;
  }

  ball_buffer_data bd = ball_buffer.data[d.ball_index];
  float dd = length(game_position(gl_FragCoord) - bd.position);

  float r_max = max(bd.dimensions.x, bd.dimensions.x - bd.line_width);
  float r_min = min(bd.dimensions.x, bd.dimensions.x - bd.line_width);
  float i_max = max(bd.dimensions.y, bd.dimensions.y + bd.line_width);
  float i_min = min(bd.dimensions.y, bd.dimensions.y + bd.line_width);

  float v0 = aa_step2(is_multisample, r_min, dd) * (1. - aa_step2(is_multisample, r_max, dd));
  float v1 = bd.dimensions.y > 0.
      ? aa_step2(is_multisample, i_min, dd) * (1. - aa_step2(is_multisample, i_max, dd))
      : 0.;
  float a = max(v0, v1);
  if (a > 0.) {
    vec4 oklab = mix(d.colour0, d.colour1, v1 / (v0 + v1));
    out_colour = vec4(oklab.xyz, a * oklab.a);
  } else {
    discard;
  }
}
