uniform sampler2D framebuffer_texture;
uniform float blend_alpha;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  vec4 c = texture(framebuffer_texture, (1. + v_texture_coords) / 2.);
  out_colour = vec4(c.xyz, c.a * blend_alpha);
}