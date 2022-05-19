#version 460

uniform sampler2D font_texture;
uniform bool font_lcd;
uniform vec4 text_colour;
in vec2 f_tex_coords;

layout(location = 0, index = 0) out vec4 output_colour;
layout(location = 0, index = 1) out vec4 output_colour_mask;

void main()
{
  if (font_lcd) {
    output_colour = vec4(text_colour.rgb, 1.);
    output_colour_mask = text_colour.a * texture(font_texture, f_tex_coords);
  } else {
    float a = texture(font_texture, f_tex_coords).r;
    output_colour = text_colour * vec4(1., 1., 1., a);
  }
}