uniform sampler2D font_texture;
uniform bool is_font_lcd;
in vec4 g_text_colour;
in vec2 g_texture_coords;

layout(location = 0, index = 0) out vec4 out_colour;
layout(location = 0, index = 1) out vec4 out_colour_mask;

void main() {
  if (is_font_lcd) {
    out_colour = vec4(g_text_colour.rgb, 1.);
    out_colour_mask = g_text_colour.a * texture(font_texture, g_texture_coords);
  } else {
    float a = texture(font_texture, g_texture_coords).r;
    out_colour = g_text_colour * vec4(1., 1., 1., a);
  }
}