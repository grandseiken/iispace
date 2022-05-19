#version 460

uniform uvec2 dimensions;
uniform ivec2 clip_min;
uniform ivec2 clip_max;
uniform uvec2 rect_dimensions;
layout(location = 0) in ivec2 position;
layout(location = 1) in ivec2 tex_coords;
out vec2 f_tex_coords;

void main()
{
  f_tex_coords = vec2(tex_coords) * vec2(rect_dimensions);
  vec2 float_position = vec2(position) / vec2(dimensions) * 2. - 1.;

  gl_ClipDistance[0] = position.x - clip_min.x;
  gl_ClipDistance[1] = position.y - clip_min.y;
  gl_ClipDistance[2] = clip_max.x - position.x;
  gl_ClipDistance[3] = clip_max.y - position.y;
  gl_Position = vec4(float_position.x, -float_position.y, 0., 1.);
}