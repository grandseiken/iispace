#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry_fragment.glsl"

uniform float colour_cycle;

flat in uint g_buffer_index;
in float g_interpolate;
out vec4 out_colour;

layout(std430, binding = 0) restrict readonly buffer shape_buffer_block {
  shape_buffer_data data[];
}
shape_buffer;

float ball_solve(vec2 v, vec2 d0, vec2 d1, float r) {
  vec2 v0 = v - d1;
  vec2 v1 = d1 - d0;

  float a = dot(v1, v1);
  float b = 2. * (dot(v, v1) - dot(d1, v1));
  float c = dot(v0, v0) - r * r;

  float sq = b * b - 4. * a * c;
  if (sq < 0.) {
    return 0.;
  }
  float s = sqrt(sq);
  float t0 = (-b - s) / (2. * a);
  float t1 = (-b + s) / (2. * a);
  return max(0., t0 * (1. - step(1., t0))) + max(0., t1 * (1. - step(1., t1)));
}

void main() {
  shape_buffer_data d0 = shape_buffer.data[g_buffer_index];
  shape_buffer_data d1 = shape_buffer.data[g_buffer_index + 1];

  if (d0.u_params.x != kStyleBall) {
    vec4 oklab0 = hsla2oklab_cycle(d0.colour, colour_cycle);
    vec4 oklab1 = hsla2oklab_cycle(d1.colour, colour_cycle);
    out_colour = mix(vec4(oklab0.xyz, .75 * oklab0.a), vec4(oklab1.xyz, 0.), g_interpolate);
    return;
  }

  // TODO: maybe need to antialias with fwidth, but actually looks OK.
  vec2 v = game_position(gl_FragCoord);
  float a = ball_solve(v, d0.position, d1.position, d0.dimensions.x);
  if (d0.dimensions.y > 0.) {
    a += ball_solve(v, d0.position, d1.position, d0.dimensions.y);
  }
  if (a > 0.) {
    // TODO: why does it cut off at top and bottom!?
    // TODO: doesn't do colour transition correctly (or even attempt to). Tricky without
    // extra passes.
    vec4 oklab0 = hsla2oklab_cycle(d0.colour, colour_cycle);
    out_colour = vec4(oklab0.xyz, min(1., .75 * a) * oklab0.a);
  } else {
    discard;
  }
}