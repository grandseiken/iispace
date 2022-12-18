#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"

uniform uvec2 screen_dimensions;

uniform vec4 position;
uniform float rotation;
uniform float interpolate;
uniform uint type0;
uniform uint type1;
uniform vec2 parameters0;
uniform vec2 parameters1;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

float scale01(float d, float t) {
  return 1. - d / 2. + t * d;
}

float noise0(vec4 v, vec2 p) {
  vec3 gradient;
  float v0 = psrdnoise3(v.xyz / 256., vec3(0.), v.w, gradient);
  float v1 = psrdnoise3(vec3(.75) + gradient / 64. + v.xyz / 128., vec3(0.), v.w * 2., gradient);
  return mix(abs(v0 + .25 * v1), abs(v0 * v1), p.x);
}

float tonemap0(float v) {
  float t0 = (5. + smoothstep(.025 - fwidth(v), .025, v)) / 6.;
  float t1 = scale01(1. / 8., smoothstep(.15 - fwidth(v), .15, v));
  return t0 * t1;
}

float noise_value(uint type, vec4 v, vec2 p) {
  switch (type) {
  case kTypeBiome0:
    return noise0(v, p);
  }
  return 0.;
}

float tone_value(uint type, float v) {
  switch (type) {
  case kTypeBiome0:
    return tonemap0(v);
    break;
  }
  return 0.;
}

float scanlines() {
  float size = max(1., round(float(screen_dimensions.y) / 540.));
  return scale01(3. / 16., floor(mod(gl_FragCoord.y, 2. * size) / size));
}

void main() {
  vec2 f_xy = g_render_dimensions * (g_texture_coords - vec2(.5));
  vec4 f_position = vec4(rotate(f_xy, rotation), 0., 0.) + position;

  float value0 = 0.;
  float value1 = 0.;
  if (interpolate < 1.) {
    value0 = noise_value(type0, f_position, parameters0);
  }
  if (interpolate > 0.) {
    value1 = noise_value(type1, f_position, parameters1);
  }
  float value = mix(value0, value1, interpolate);

  float tone0 = 0.;
  float tone1 = 0.;
  if (interpolate < 1.) {
    tone0 = tone_value(type0, value);
  }
  if (interpolate > 0.) {
    tone1 = tone_value(type1, value);
  }
  float tone = mix(tone0, tone1, interpolate);
  out_colour = vec4(min(1., tone * scanlines() * g_colour.x), g_colour.yz, 1.);
}
