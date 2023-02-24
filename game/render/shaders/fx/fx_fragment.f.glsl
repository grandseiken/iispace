#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"
#include "game/render/shaders/lib/position_fragment.glsl"

const float kInverseSqrt2 = .70710678118;

flat in float g_time;
flat in float g_rotation;
flat in uint g_shape;
flat in uint g_style;
flat in vec4 g_colour;
flat in vec2 g_position;
flat in vec2 g_dimensions;
flat in vec2 g_seed;

out vec4 out_colour;
out vec4 out_mask;

float scale01(float d, float t) {
  return 1. - d / 2. + t * d;
}

// TODO: gradient unused.
float noise0(vec3 v, out vec2 g) {
  vec2 g0;
  float v0 = psrdnoise2(v.xy / 64., vec2(0.), v.z / 64., g0);
  vec2 g1;
  float v1 = psrdnoise2(vec2(.75) + g0 / 4. + v.xy / 24., vec2(0.), v.z / 64., g1);
  g = sign(v0 + .25 * v1) * (g0 / 64. + .25 * g1 / 24.);
  return abs(v0 + .25 * v1);
}

float ball_coefficient(vec2 v) {
  float d = length(v - g_position);
  float r0 = g_dimensions.y;
  float r1 = g_dimensions.x;
  return -4. * (d - r0) * (d - r1) / ((r1 - r0) * (r1 - r0));
}

float box_coefficient(vec2 v) {
  vec2 d = abs(rotate(v - g_position, -g_rotation));
  float t = min(g_dimensions.x, g_dimensions.y);
  return min(1., min((g_dimensions.x - d.x) / t, (g_dimensions.y - d.y) / t));
}

float shape_coefficient(vec2 v) {
  switch (g_shape) {
  case kFxShapeBall:
    return ball_coefficient(v);
  case kFxShapeBox:
    return box_coefficient(v);
  }
  return 0.;
}

void main() {
  vec2 frag_position = game_position(gl_FragCoord);

  switch (g_style) {
  case kFxStyleExplosion: {
    vec2 gradient;
    // TODO: position below maybe needs to be rotated for box case.
    float n = noise0(vec3(frag_position + g_seed - g_position, g_seed.x + g_time), gradient);
    float q = g_colour.a * clamp(shape_coefficient(frag_position), 0., 1.);

    float v = n * q * 16. + q;
    float a = smoothstep(1. - fwidth(v), 1., v);
    out_colour = vec4(g_colour.xyz, a);
    break;
  }

  default:
    out_colour = g_colour;
    break;
  }
  out_mask = vec4(1., 0., 0., out_colour.a);
}