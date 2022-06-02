#version 460

uniform sampler2D font_texture;
uniform bool is_font_lcd;
uniform vec4 text_colour;
in vec2 v_texture_coords;

layout(location = 0, index = 0) out vec4 out_colour;
layout(location = 0, index = 1) out vec4 out_colour_mask;

void main()
{
  if (is_font_lcd) {
    out_colour = vec4(text_colour.rgb, 1.);
    out_colour_mask = text_colour.a * texture(font_texture, v_texture_coords);
  } else {
    float a = texture(font_texture, v_texture_coords).r;
    out_colour = text_colour * vec4(1., 1., 1., a);
  }
}