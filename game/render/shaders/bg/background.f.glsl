#include "external/psrdnoise/src/psrdnoise3.glsl"

const float kPi = 3.1415926538;

uniform float tick_count;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

// float poster(float v, float d) {
//   float a = abs(v);
//   float t = mod(a, d) / d;
//   t = smoothstep(0., 1., smoothstep(0., 1., smoothstep(0., 1., t)));
//   return sign(v) * (a - mod(a, d) + t * d);
// }

float scale01(float d, float t) {
  return 1. - d / 2. + t * d;
}

void main() {
  vec3 period = vec3(0.);
  vec2 vv = g_render_dimensions * g_texture_coords - .5 * g_render_dimensions;
  vec3 xy =
      vec3(vv.xy, 0.) + vec3(256. * cos(tick_count / 512.), -tick_count / 4., tick_count / 8.);
  vec3 gradient;
  float v = 0.;
  v += 1. * psrdnoise(xy / 256., period, tick_count / 256., gradient);
  v += .25 *
      psrdnoise(vec3(.75, .75, .75) + gradient / 64. + xy / 128., period, tick_count / 128.,
                gradient);
  v = abs(v);
  float t0 = .75 + .25 * smoothstep(.025 - fwidth(v), .025, v);
  float t1 = scale01(.25, smoothstep(.15 - fwidth(v), .15, v));
  float t2 = scale01(.5, floor(mod(g_render_dimensions.y * g_texture_coords.y, 2.)));
  out_colour = vec4(t0 * t1 * t2 * g_colour.rgb, 1.);
}