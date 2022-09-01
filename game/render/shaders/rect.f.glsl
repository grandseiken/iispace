#include "game/render/shaders/lib/hsl.glsl"

uniform uvec2 rect_dimensions;
uniform vec4 rect_colour_lo;
uniform vec4 rect_colour_hi;
uniform vec4 border_colour_lo;
uniform vec4 border_colour_hi;
uniform uvec2 border_size;
in vec2 v_texture_coords;
out vec4 out_colour;

vec4 ungamma(vec4 v) {
  return vec4(pow(v.rgb, vec3(1. / 2.2)), v.a);
}

vec4 gamma(vec4 v) {
  return vec4(pow(v.rgb, vec3(2.2)), v.a);
}

void main() {
  bool is_border = any(lessThan(v_texture_coords, vec2(border_size))) ||
      any(greaterThan(v_texture_coords, vec2(rect_dimensions) - border_size));
  vec2 v = v_texture_coords / vec2(rect_dimensions);

  vec4 inner_colour =
      mix(ungamma(hsl2rgba(rect_colour_lo)), ungamma(hsl2rgba(rect_colour_hi)), (v.x + v.y) / 2.);
  vec4 border_colour = mix(ungamma(hsl2rgba(border_colour_lo)), ungamma(hsl2rgba(border_colour_hi)),
                           (v.x + v.y) / 2.);

  out_colour = gamma(mix(inner_colour, border_colour, float(is_border)));
}