#include "game/render/shaders/lib/hsl.glsl"
#include "game/render/shaders/shape/data.glsl"

uniform float colour_cycle;

layout(location = 0) in uint in_style;
layout(location = 1) in uvec2 in_params;
layout(location = 2) in float in_rotation;
layout(location = 3) in float in_line_width;
layout(location = 4) in vec3 in_position;
layout(location = 5) in vec2 in_dimensions;
layout(location = 6) in vec4 in_colour;

out v_out_t {
  shape_data data;
}
v_out;

void main() {
  v_out.data.style = in_style;
  v_out.data.params = in_params;
  v_out.data.rotation = in_rotation;
  v_out.data.line_width = in_line_width;
  v_out.data.dimensions = in_dimensions;
  v_out.data.colour = hsl2rgba_cycle(in_colour, colour_cycle);
  gl_Position = vec4(in_position, 1.);
}