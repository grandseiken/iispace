#version 460
#include "game/render/shaders/lib/hsl.glsl"

uniform vec2 render_scale;
uniform uvec2 render_dimensions;
uniform uvec2 texture_dimensions;
uniform vec4 text_colour;
uniform float colour_cycle;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_texture_coords;
out vec2 v_texture_coords;
out vec4 v_text_colour;

void main() {
  v_texture_coords = vec2(in_texture_coords) / vec2(texture_dimensions);
  v_text_colour = hsl2rgba_cycle(text_colour, colour_cycle);
  vec2 render_position = render_scale * (2. * in_position / vec2(render_dimensions) - 1.);
  gl_Position = vec4(render_position.x, -render_position.y, 0., 1.);
}