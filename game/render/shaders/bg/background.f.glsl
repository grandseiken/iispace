#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/frag_math.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"

const float kRenderHeight = 540.;
const float kPolarPeriod = 256.;

uniform bool is_multisample;
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

float noise0_polar(vec4 v, vec2 p) {
  vec3 gradient;
  float v0 = psrdnoise3(v.xyz / vec3(256., 64., 256.), vec3(0., 4., 0.), v.w, gradient);
  float v1 = psrdnoise3(vec3(.75) + gradient / 64. + v.xyz / vec3(128., 32., 128.),
                        vec3(0., 8., 0.), v.w * 2., gradient);
  return mix(abs(v0 + .25 * v1), abs(v0 * v1), p.x);
}

float tonemap0(float v) {
  float t0 = (5. + aa_step(is_multisample, .025, v)) / 6.;
  float t1 = scale01(1. / 8., aa_step(is_multisample, .15, v));
  return t0 * t1;
}

float noise_value(uint type, vec4 v, vec4 v_polar, vec2 p) {
  switch (type) {
  case kBgTypeBiome0:
    return noise0(v, p);
  case kBgTypeBiome0_Polar:
    return noise0_polar(v_polar, p);
  }
  return 0.;
}

float tone_value(uint type, float v) {
  switch (type) {
  case kBgTypeBiome0:
  case kBgTypeBiome0_Polar:
    return tonemap0(v);
    break;
  }
  return 0.;
}

float scanlines() {
  float size = max(1., round(float(screen_dimensions.y) / kRenderHeight));
  return scale01(3. / 16., floor(mod(gl_FragCoord.y, 2. * size) / size));
}

void main() {
  vec2 f_xy = rotate(g_render_dimensions * (g_texture_coords - vec2(.5)), rotation);
  vec4 f_position = position + vec4(f_xy, 0., 0.);
  vec4 f_polar = vec4(position.x, position.y / (2. * kPi), position.zw) +
      vec4(length(f_xy), kPolarPeriod * atan(f_xy.y, f_xy.x) / (2. * kPi), 0., 0.);

  float value0 = 0.;
  float value1 = 0.;
  if (interpolate < 1.) {
    value0 = noise_value(type0, f_position, f_polar, parameters0);
  }
  if (interpolate > 0.) {
    value1 = noise_value(type1, f_position, f_polar, parameters1);
  }
  // TODO: potentially alternate interpolation modes, e.g.:
  // const float kRadialTransition = .125;
  // float v = (1. - kRadialTransition) * length(f_xy) / length(g_render_dimensions / 2.);
  // float value =
  //     mix(value0, value1, 1. - smoothstep(interpolate - kRadialTransition, interpolate, v));
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
