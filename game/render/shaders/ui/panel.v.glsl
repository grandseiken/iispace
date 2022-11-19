#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/ui/data.glsl"

uniform float colour_cycle;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_screen_dimensions;
layout(location = 2) in ivec2 in_render_dimensions;
layout(location = 3) in vec4 in_panel_colour;
layout(location = 4) in int in_style;

out v_out_t {
  panel_data data;
}
v_out;

void main() {
  v_out.data.screen_dimensions = in_screen_dimensions;
  v_out.data.render_dimensions = in_render_dimensions;
  v_out.data.style = in_style;
  v_out.data.panel_colour = hsla2oklab_cycle(in_panel_colour, colour_cycle);
  gl_Position = vec4(in_position.xy, 0., 1.);
}
