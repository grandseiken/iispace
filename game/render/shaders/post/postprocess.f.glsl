#include "game/render/shaders/lib/oklab.glsl"

uniform uvec2 screen_dimensions;
uniform uint framebuffer_samples;
uniform sampler2D framebuffer_texture;
uniform sampler2DMS framebuffer_texture_multisample;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  if (framebuffer_samples <= 1) {
    out_colour =
        vec4(oklab2srgb(texture(framebuffer_texture, (1. + v_texture_coords) / 2.).xyz), 1.);
  } else {
    vec3 v = vec3(0.);
    ivec2 c = ivec2(screen_dimensions * (1 + v_texture_coords) / 2.);
    for (int i = 0; i < framebuffer_samples; ++i) {
      v += clamp(oklab2rgb(texelFetch(framebuffer_texture_multisample, c, i).xyz), vec3(0.),
                 vec3(1.));
    }
    // TODO: is this still introducing weird pixel glitches? Could it be due to not clamping oklab
    // outputs earlier?
    out_colour = vec4(clamp(rgb2srgb(v / float(framebuffer_samples)), vec3(0.), vec3(1.)), 1.);
  }
}