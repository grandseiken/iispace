#include "game/render/shaders/bg/data.glsl"
#include "game/render/shaders/lib/frag_math.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"

const float kRenderHeight = 540.;
const float kPolarVelocityMultiplier = 1. / 1024.;

uniform bool is_multisample;
uniform uvec2 screen_dimensions;

uniform vec4 position;
uniform float rotation;
uniform float interpolate;

uniform bg_spec spec0;
uniform bg_spec spec1;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

float offset_centred(float d, float t) {
  return 1. - d / 2. + t * d;
}

vec2 height_turbulent0(bg_spec spec, float polar_period, vec4 v) {
  vec3 gradient;
  vec3 period = {0., polar_period, 0.};
  float v0 = psrdnoise3(v.xyz / spec.scale, period, v.w, gradient);
  float v1 = psrdnoise3(vec3(.75) + 4. * gradient / spec.scale + 2. * v.xyz / spec.scale,
                        2. * period, 2. * v.w, gradient);
  return vec2(v0, v1);
}

float tonemap_tendrils(float v, float t) {
  float t0 = (5. + aa_step_upper(is_multisample, .025, v)) / 6.;
  float t1 = offset_centred(1. / 8., aa_step_upper(is_multisample, .15, v));
  return t0 * t1;
}

float tonemap_sin_combo(float v, float t) {
  float t0 = (7. + aa_step_centred(is_multisample, 0., sin(43. * v))) / 8.;
  return t0 * (1. - aa_step_centred(is_multisample, 0., sin(t + 11. * v)) / 6.);
}

vec2 height_values(bg_spec spec, vec4 v, vec4 v_polar) {
  v_polar *= vec4(1., spec.polar_period * spec.scale, 1., 1.);
  switch (spec.height_function) {
  case kHeightFunction_Zero:
    return vec2(0.);
  case kHeightFunction_Turbulent0:
    return spec.polar_period != 0. ? height_turbulent0(spec, spec.polar_period, v_polar)
                                   : height_turbulent0(spec, 0., v);
  }
  return vec2(0.);
}

float combinator(bg_spec spec, vec2 v) {
  switch (spec.combinator) {
  case kCombinator_Octave0:
    return mix(v.x + spec.persistence * v.y, v.x * v.y, spec.parameters.x);
  case kCombinator_Abs0:
    return mix(abs(v.x + spec.persistence * v.y), abs(v.x * v.y), spec.parameters.x);
  }
  return 0.;
}

float tone_value(bg_spec spec, float v, float t) {
  switch (spec.tonemap) {
  case kTonemap_Passthrough:
    return clamp(v, 0., 1.);
  case kTonemap_Tendrils:
    return tonemap_tendrils(v, t);
  case kTonemap_SinCombo:
    return tonemap_sin_combo(v, t);
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
  vec4 f_polar = position * vec4(1., kPolarVelocityMultiplier, 1., 1.) +
      vec4(length(f_xy), atan(f_xy.y, f_xy.x) / (2. * kPi), 0., 0.);

  float value0 = 0.;
  float value1 = 0.;
  if (interpolate < 1.) {
    value0 = combinator(spec0, height_values(spec0, f_position, f_polar));
  }
  if (interpolate > 0.) {
    value1 = combinator(spec1, height_values(spec1, f_position, f_polar));
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
    tone0 = tone_value(spec0, value, position.w);
  }
  if (interpolate > 0.) {
    tone1 = tone_value(spec1, value, position.w);
  }
  float tone = mix(tone0, tone1, interpolate);
  out_colour = vec4(min(1., tone * scanlines() * g_colour.x), g_colour.yz, 1.);
}
