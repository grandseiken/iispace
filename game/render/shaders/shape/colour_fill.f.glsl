#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry_fragment.glsl"

uniform float colour_cycle;

flat in uint g_buffer_index;
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
    out_colour = hsla2oklab_cycle(d.colour, colour_cycle);
    return;
  }

  ball_buffer_data bd = ball_buffer.data[d.ball_index];
  float dd = length(game_position(gl_FragCoord) - bd.position);
  float wd = fwidth(dd);

  float r_max = bd.dimensions.x;
  float r_min = bd.dimensions.y;
  float a = smoothstep(r_min - wd, r_min + wd, dd) * (1. - smoothstep(r_max - wd, r_max + wd, dd));

  if (a > 0.) {
    vec4 oklab = hsla2oklab_cycle(d.colour, colour_cycle);
    out_colour = vec4(oklab.xyz, a * oklab.a);
  } else {
    discard;
  }
}