#version 460

uniform vec2 render_scale;
uniform uvec2 render_dimensions;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_colour;
out vec4 v_line_colour;

void main()
{
  v_line_colour = in_colour;
  vec2 render_position = render_scale * (2. * in_position / vec2(render_dimensions) - 1.);
  gl_Position = vec4(render_position.x, -render_position.y, 0., 1.);
}