#version 460

uniform sampler2D font_texture;
uniform vec4 text_colour;
in vec2 f_tex_coords;

layout(location = 0, index = 0) out vec4 output_colour;

void main()
{
  float a = texture(font_texture, f_tex_coords).r;
  output_colour = text_colour * vec4(1., 1., 1., a);
}