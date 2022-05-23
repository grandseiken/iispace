#version 460

uniform uvec2 rect_dimensions;
uniform uvec2 line_width;
uniform vec4 rect_colour;
in vec2 f_tex_coords;
out vec4 output_colour;

void main()
{
  bool is_border = any(lessThan(f_tex_coords, vec2(line_width))) ||
      any(greaterThan(f_tex_coords, vec2(rect_dimensions) - line_width));
  output_colour = mix(vec4(0.), rect_colour, float(is_border));
}