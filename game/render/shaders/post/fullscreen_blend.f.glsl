uniform float blend_alpha;
uniform sampler2D framebuffer_texture;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  vec3 source = texture(framebuffer_texture, (1. + v_texture_coords) / 2.).xyz;
  out_colour = vec4(source, blend_alpha);
}