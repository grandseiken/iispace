#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry_fragment.glsl"

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
  if (d.style != kStyleBall) {
    out_colour = mix(d.colour0, d.colour1, g_colour_interpolate);
    return;
  }

  ball_buffer_data bd = ball_buffer.data[d.ball_index];
  float dd = length(game_position(gl_FragCoord) - bd.position);
  float wd = fwidth(dd);

  float r_max = max(bd.dimensions.x, bd.dimensions.x - bd.line_width);
  float r_min = min(bd.dimensions.x, bd.dimensions.x - bd.line_width);
  float i_max = max(bd.dimensions.y, bd.dimensions.y + bd.line_width);
  float i_min = min(bd.dimensions.y, bd.dimensions.y + bd.line_width);

  float v0 = smoothstep(r_min - wd, r_min + wd, dd) * (1. - smoothstep(r_max - wd, r_max + wd, dd));
  float v1 = bd.dimensions.y > 0.
      ? smoothstep(i_min - wd, i_min + wd, dd) * (1. - smoothstep(i_max - wd, i_max + wd, dd))
      : 0.;
  float a = max(v0, v1);
  if (a > 0.) {
    vec4 oklab = mix(d.colour0, d.colour1, v1 / (v0 + v1));
    out_colour = vec4(oklab.xyz, a * oklab.a);
  } else {
    discard;
  }
}