#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"
#include "game/render/shaders/lib/position_fragment.glsl"

const float kInverseSqrt2 = .70710678118;

flat in float g_time;
flat in uint g_style;
flat in vec4 g_colour;
flat in vec2 g_position;
flat in vec2 g_dimensions;
flat in vec2 g_seed;

out vec4 out_colour;

float scale01(float d, float t) {
  return 1. - d / 2. + t * d;
}

// TODO: gradient unused.
float noise0(vec4 v, out vec2 g) {
  vec3 g0;
  float v0 = psrdnoise3(v.xyz / 64., vec3(0.), v.w / 64., g0);
  vec3 g1;
  float v1 = psrdnoise3(vec3(.75) + g0 / 4. + v.xyz / 32., vec3(0.), v.w / 32., g1);
  g = sign(v0 + .25 * v1) * (g0.xy / 64. + .25 * g1.xy / 32.);
  return abs(v0 + .25 * v1);
}

float ball_coefficient(vec2 v) {
  float d = length(v - g_position);
  float r0 = g_dimensions.y;
  float r1 = g_dimensions.x;
  return -4. * (d - r0) * (d - r1) / ((r1 - r0) * (r1 - r0));
}

void main() {
  vec2 frag_position = game_position(gl_FragCoord);

  switch (g_style) {
  case kFxStyleExplosion: {
    vec2 gradient;
    float n =
        noise0(vec4(frag_position + g_seed - g_position, g_time, g_seed.x + g_time), gradient);
    float q = clamp(g_colour.a * ball_coefficient(frag_position), 0., 1.);

    float v = n * q * 16.;
    float a = smoothstep(1. - fwidth(v), 1., v);
    out_colour = vec4(g_colour.xyz, a);
    break;
  }

  default:
    out_colour = g_colour;
    break;
  }
}