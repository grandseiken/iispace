#version 460

uniform uvec2 rect_dimensions;
uniform vec4 rect_colour1;
uniform vec4 rect_colour2;
uniform vec4 border_colour1;
uniform vec4 border_colour2;
uniform uvec2 border_size;
uniform uint vertical_gradient;
in vec2 f_tex_coords;
out vec4 output_colour;

vec4 ungamma(vec4 v) {
  return vec4(pow(v.rgb, vec3(1. / 2.2)), v.a);
}

vec4 gamma(vec4 v) {
  return vec4(pow(v.rgb, vec3(2.2)), v.a);
}

void main()
{
  bool is_border = any(lessThan(f_tex_coords, vec2(border_size))) ||
      any(greaterThan(f_tex_coords, vec2(rect_dimensions) - border_size));
  vec2 v = f_tex_coords / vec2(rect_dimensions);

  vec4 inner_colour = mix(
      ungamma(rect_colour1), ungamma(rect_colour2),
      dot(v, vec2(1 - vertical_gradient, vertical_gradient)));
  vec4 border_colour = mix(ungamma(border_colour1), ungamma(border_colour2), (v.x + v.y) / 2.);

  output_colour = gamma(mix(inner_colour, border_colour, float(is_border)));
}