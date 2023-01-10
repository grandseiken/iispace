#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/oklab.glsl"

uniform float colour_cycle;

layout(location = 0) in uint in_style;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in vec2 in_position;
layout(location = 3) in vec2 in_dimensions;
layout(location = 4) in vec2 in_seed;

out v_out_t {
  fx_vertex_data data;
}
v_out;

void main() {
  v_out.data.style = in_style;
  v_out.data.colour = hsla2oklab_cycle(in_colour, colour_cycle);
  v_out.data.dimensions = in_dimensions;
  v_out.data.seed = in_seed;
  gl_Position = vec4(in_position, 1., 1.);
}