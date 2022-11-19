#include "game/render/shaders/lib/hsl.glsl"

uniform sampler2D framebuffer_texture;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  out_colour = vec4(rgb2srgb(texture(framebuffer_texture, (1. + v_texture_coords) / 2.).rgb), 1.);
}