#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/frag_math.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"

const float kRenderHeight = 540.;
const float kPolarPeriod = 1024.;

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

float offset_centred(float d, float t) {
  return 1. - d / 2. + t * d;
}

// m controls how "big" the noise is. Lower values means more detail.
float scale_m(float octave, float m, float x) {
  return octave * x / m;
}
vec3 scale_m(float octave, float m, vec3 x) {
  return octave * x / m;
}
float scale(float octave, float x) {
  return octave * x;
}

// TODO: factor things out a bit for reuse. Introduce a struct for each BG setting with various
// parameters (e.g. polar, multiplier m, tonemap setting, noise function, etc).
// Split layered noise function, combination function (sometime non-abs is better), tone function.
float noise0(vec2 parameters, float m, float polar_period, vec4 v) {
  vec3 gradient;
  vec3 period = vec3(0., polar_period, 0.);
  float v0 = psrdnoise3(scale_m(1., m, v.xyz), scale_m(1., m, period), scale(1., v.w), gradient);
  float v1 = psrdnoise3(vec3(.75) + scale_m(4., m, gradient) + scale_m(2., m, v.xyz),
                        scale_m(2., m, period), scale(2., v.w), gradient);
  return mix(abs(v0 + .25 * v1), abs(v0 * v1), parameters.x);
}

float tonemap0(float v, float t) {
  float t0 = (5. + aa_step_upper(is_multisample, .025, v)) / 6.;
  float t1 = offset_centred(1. / 8., aa_step_upper(is_multisample, .15, v));
  return t0 * t1;
}

float tonemap1(float v, float t) {
  float t0 = (7. + aa_step_centred(is_multisample, 0., sin(43. * v))) / 8.;
  return t0 * (1. - aa_step_centred(is_multisample, 0., sin(t + 11. * v)) / 6.);
}

float noise_value(uint type, vec2 parameters, vec4 v, vec4 v_polar) {
  switch (type) {
  case kBgTypeBiome0:
    return noise0(parameters, 512., 0., v);
  case kBgTypeBiome0_Polar:
    return noise0(parameters, 512., kPolarPeriod, v_polar);
  }
  return 0.;
}

float tone_value(uint type, vec2 parameters, float v, float t) {
  switch (type) {
  case kBgTypeBiome0:
    // Previously: tonemap0 amd noise0 with m = 256. Might look better without abs?
    return tonemap1(v, t);
  case kBgTypeBiome0_Polar:
    return tonemap1(v, t);
  }
  return 0.;
}

float scanlines() {
  float size = max(1., round(float(screen_dimensions.y) / kRenderHeight));
  return offset_centred(3. / 16., floor(mod(gl_FragCoord.y, 2. * size) / size));
}

void main() {
  vec2 screen_position = g_texture_coords - vec2(.5);
  vec2 f_xy = rotate(g_render_dimensions * screen_position, rotation);
  vec4 f_position = position + vec4(f_xy, 0., 0.);
  vec4 f_polar =
      position + vec4(length(f_xy), kPolarPeriod * atan(f_xy.y, f_xy.x) / (2. * kPi), 0., 0.);

  float value0 = 0.;
  float value1 = 0.;
  if (interpolate < 1.) {
    value0 = noise_value(type0, parameters0, f_position, f_polar);
  }
  if (interpolate > 0.) {
    value1 = noise_value(type1, parameters1, f_position, f_polar);
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
    tone0 = tone_value(type0, parameters0, value, position.w);
  }
  if (interpolate > 0.) {
    tone1 = tone_value(type1, parameters1, value, position.w);
  }
  float tone = mix(tone0, tone1, interpolate);
  out_colour = vec4(min(1., tone * scanlines() * g_colour.x), g_colour.yz, 1.);
}
