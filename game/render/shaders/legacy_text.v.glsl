#version 460

uniform uvec2 screen_dimensions;
uniform uvec2 render_dimensions;
uniform uvec2 texture_dimensions;
layout(location = 0) in ivec2 position;
layout(location = 1) in ivec2 tex_coords;
out vec2 f_tex_coords;

void main()
{
  vec2 f_screen = vec2(screen_dimensions);
  vec2 f_render = vec2(render_dimensions);
  float screen_aspect = f_screen.x / f_screen.y;
  float render_aspect = f_render.x / f_render.y;

  vec2 scale;
  if (screen_aspect > render_aspect) {
    scale = vec2(render_aspect / screen_aspect, 1.);
  } else {
    scale = vec2(1., screen_aspect / render_aspect);
  }

  vec2 float_tex_coords = vec2(tex_coords) / vec2(texture_dimensions);
  f_tex_coords = vec2(float_tex_coords.x, float_tex_coords.y);
  vec2 float_position = scale * (2. * vec2(position) / vec2(render_dimensions) - 1.);
  gl_Position = vec4(float_position.x, -float_position.y, 0., 1.);
}