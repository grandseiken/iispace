#include "game/render/shaders/lib/position_fragment.glsl"
#include "game/render/shaders/shape/data.glsl"

flat in uint g_buffer_index;
centroid in float g_interpolate;
centroid in float g_colour_interpolate;
out vec4 out_colour;

layout(std430, binding = 0) restrict readonly buffer shape_buffer_block {
  shape_buffer_data data[];
}
shape_buffer;

layout(std430, binding = 1) restrict readonly buffer ball_buffer_block {
  ball_buffer_data data[];
}
ball_buffer;

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

  if (d0.style != kShapeStyle_Ball) {
    vec4 oklab0 = mix(d0.colour0, d0.colour1, g_colour_interpolate);
    vec4 oklab1 = mix(d1.colour0, d1.colour1, g_colour_interpolate);
    out_colour = mix(vec4(oklab0.xyz, .75 * oklab0.a), vec4(oklab1.xyz, 0.), g_interpolate);
    return;
  }

  ball_buffer_data bd0 = ball_buffer.data[d0.ball_index];
  ball_buffer_data bd1 = ball_buffer.data[d1.ball_index];
  // TODO: maybe need to antialias with fwidth, but actually looks OK?
  vec2 vp = game_position(gl_FragCoord);
  float v0 = ball_solve(vp, bd0.position, bd1.position, bd0.dimensions.x - min(bd0.line_width, 0.));
  float v1 = 0.;
  if (bd0.dimensions.y > 0.) {
    v1 = ball_solve(vp, bd0.position, bd1.position, bd0.dimensions.y + min(bd0.line_width, 0.));
  }
  if (v0 + v1 <= 0.) {
    discard;
    return;
  }
  vec4 oklab0 = mix(d1.colour0, d0.colour0, min(1., v0));
  vec4 oklab1 = mix(d1.colour1, d0.colour1, min(1., v1));
  out_colour = vec4(mix(oklab0.xyz, oklab1.xyz, v1 / (v0 + v1)),
                    min(1., .75 * (v0 * oklab0.a + v1 * oklab1.a)));
}