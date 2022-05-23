#version 460

uniform vec2 render_scale;
uniform uvec2 render_dimensions;
uniform uvec2 rect_dimensions;
layout(location = 0) in ivec2 position;
layout(location = 1) in ivec2 tex_coords;
out vec2 f_tex_coords;

void main()
{
  f_tex_coords = vec2(tex_coords) * vec2(rect_dimensions);
  vec2 float_position = render_scale * (2. * vec2(position) / vec2(render_dimensions) - 1.);
  gl_Position = vec4(float_position.x, -float_position.y, 0., 1.);
}