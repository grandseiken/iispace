#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/noise/simplex.glsl"
#include "game/render/shaders/lib/position_fragment.glsl"

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

float noise0(vec4 v) {
  vec3 gradient;
  float v0 = psrdnoise3(v.xyz / 64., vec3(0.), v.w / 64., gradient);
  float v1 = psrdnoise3(vec3(.75) + gradient / 4. + v.xyz / 32., vec3(0.), v.w / 32., gradient);
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
    float q = clamp(g_colour.a * ball_coefficient(frag_position), 0., 1.);
    q *= noise0(vec4(frag_position + g_seed - g_position, g_time, g_seed.x + g_time));
    float v = q * 16.;
    float a = smoothstep(1. - fwidth(v), 1., v);
    float v0 = smoothstep(2. - fwidth(v), 2., v);
    float v1 = smoothstep(3.5 - fwidth(v), 3.5, v);
    float v2 = smoothstep(5. - fwidth(v), 5., v);
    vec3 c = mix(vec3(0.), vec3(.75 * g_colour.x, g_colour.yz), v0);
    c = mix(c, g_colour.xyz, v1);
    c = mix(c, vec3((g_colour.x + 1.) / 2., g_colour.yz), v2);
    out_colour = vec4(c.xyz, a);
    gl_FragDepth = clamp(v / 5., 0., 1.);
    break;
  }

  default:
    out_colour = g_colour;
    break;
  }
}