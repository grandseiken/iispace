#include "game/render/shaders/lib/noise/simplex.glsl"

const float kPi = 3.1415926538;

uniform vec4 offset;
uniform vec2 parameters;
uniform uvec2 screen_dimensions;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

float scale01(float d, float t) {
  return 1. - d / 2. + t * d;
}

float noise0(vec3 xy, float a, float t) {
  vec3 gradient;
  float v0 = psrdnoise3(xy / 256., vec3(0.), a, gradient);
  float v1 = psrdnoise3(vec3(.75) + gradient / 64. + xy / 128., vec3(0.), a * 2., gradient);
  return mix(abs(v0 + .25 * v1), abs(v0 * v1), t);
}

float tonemap0(float v) {
  float t0 = (5. + smoothstep(.025 - fwidth(v), .025, v)) / 6.;
  float t1 = scale01(1./ 8., smoothstep(.15 - fwidth(v), .15, v));
  return t0 * t1;
}

float scanlines() {
  float size = max(1., round(screen_dimensions.y / 540.));
  return scale01(3. / 16., floor(mod(gl_FragCoord.y, 2. * size) / size));
}

void main() {
  vec2 vv = g_render_dimensions * (g_texture_coords - vec2(.5));
  vec3 xy = vec3(vv.xy, 0.) + offset.xyz;

  float v = noise0(xy, offset.a, parameters.x);
  out_colour = vec4(min(1., tonemap0(v) * scanlines() * g_colour.x), g_colour.yz, 1.);
}