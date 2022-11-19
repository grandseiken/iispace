#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/oklab.glsl"

uniform float colour_cycle;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_screen_dimensions;
layout(location = 2) in ivec2 in_render_dimensions;
layout(location = 3) in vec4 in_colour;

out v_out_t {
  background_data data;
}
v_out;

void main() {
  v_out.data.screen_dimensions = in_screen_dimensions;
  v_out.data.render_dimensions = in_render_dimensions;
  v_out.data.colour = hsla2oklab_cycle(in_colour, colour_cycle);
  gl_Position = vec4(in_position.xy, 0., 1.);
}
