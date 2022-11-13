#include "external/psrdnoise/src/psrdnoise2.glsl"

const float kPi = 3.1415926538;

uniform float tick_count;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

float poster(float v, float d) {
  float a = abs(v);
  float t = mod(a, d) / d;
  t = smoothstep(0., 1., smoothstep(0., 1., smoothstep(0., 1., t)));
  return sign(v) * (a - mod(a, d) + t * d);
}

void main() {
  vec2 period = vec2(0., 32.);
  vec2 vv = g_render_dimensions * g_texture_coords - .5 * g_render_dimensions;
  vec2 xy = vec2(length(vv), 128 * period.y * (kPi + atan(vv.y, vv.x)) / (2. * kPi)) +
      vec2(tick_count * 2, sin(tick_count / 64) * length(vv));
  vec2 gradient;
  float v = 0.;
  float a = tick_count / 48.;
  v += psrdnoise(xy / 64., period, a, gradient) / 1.;
  v += psrdnoise(.1 + xy / 128., period, a * 1.5, gradient) / 2.;
  v += psrdnoise(.2 + xy / 64., period, a * 2., gradient) / 4.;
  v += psrdnoise(.4 + xy / 8., period, a * 2.5, gradient) / 8.;
  v += psrdnoise(.5 + xy / 2., period, a * 4., gradient) / 4.;
  out_colour = vec4(g_colour.rgb * (1.25 - .5 * abs(v)), 1.);
}