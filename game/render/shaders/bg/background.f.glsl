uniform sampler3D noise_texture;

flat in vec2 g_render_dimensions;
flat in vec4 g_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

void main() {
  float v = texture(noise_texture, vec3(g_texture_coords, 0)).r;
  out_colour = vec4(g_colour.rgb * (1. + v), 1.);
}