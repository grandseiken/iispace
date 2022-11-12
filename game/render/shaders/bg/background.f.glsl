uniform sampler2D noise_texture;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

void main() {
  out_colour = g_colour;
}