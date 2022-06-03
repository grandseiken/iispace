#version 460

uniform vec2 render_scale;
uniform uvec2 render_dimensions;
uniform uvec2 rect_dimensions;
layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_texture_coords;
out vec2 v_texture_coords;

void main() {
  v_texture_coords = vec2(in_texture_coords) * vec2(rect_dimensions);
  vec2 render_position = render_scale * (2. * in_position / vec2(render_dimensions) - 1.);
  gl_Position = vec4(render_position.x, -render_position.y, 0., 1.);
}