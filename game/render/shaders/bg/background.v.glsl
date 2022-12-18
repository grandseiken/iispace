#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/oklab.glsl"

uniform float colour_cycle;
uniform float interpolate;
uniform vec4 colour0;
uniform vec4 colour1;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_screen_dimensions;
layout(location = 2) in ivec2 in_render_dimensions;

out v_out_t {
  background_data data;
}
v_out;

void main() {
  v_out.data.screen_dimensions = in_screen_dimensions;
  v_out.data.render_dimensions = in_render_dimensions;
  v_out.data.colour = mix(hsla2oklab(colour0), hsla2oklab(colour1), interpolate);
  gl_Position = vec4(in_position.xy, 0., 1.);
}
