#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/ui/data.glsl"

uniform vec4 text_colour;
uniform float colour_cycle;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_dimensions;
layout(location = 2) in ivec2 in_texture_coords;

out v_out_t {
  glyph_data data;
}
v_out;

void main() {
  v_out.data.texture_coords = in_texture_coords;
  v_out.data.dimensions = in_dimensions;
  v_out.data.text_colour = hsla2oklab_cycle(text_colour, colour_cycle);
  gl_Position = vec4(in_position.xy, 0., 1.);
}