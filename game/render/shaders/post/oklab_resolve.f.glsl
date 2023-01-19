#include "game/render/shaders/lib/oklab.glsl"

uniform uvec2 screen_dimensions;
uniform bool is_multisample;
uniform bool is_output_srgb;
uniform sampler2D framebuffer_texture;
uniform sampler2DMS framebuffer_texture_multisample;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  if (!is_multisample) {
    vec3 oklab = texture(framebuffer_texture, (1. + v_texture_coords) / 2.).xyz;
    if (is_output_srgb) {
      out_colour = vec4(oklab2srgb(oklab), 1.);
    } else {
      out_colour = vec4(oklab, 1.);
    }
    return;
  }

  ivec2 c = ivec2(screen_dimensions * (1 + v_texture_coords) / 2.);
  int samples = textureSamples(framebuffer_texture_multisample);
  vec3 v = vec3(0.);
  for (int i = 0; i < samples; ++i) {
    v += oklab2rgb(texelFetch(framebuffer_texture_multisample, c, i).xyz);
  }
  v /= float(samples);
  if (is_output_srgb) {
    out_colour = vec4(rgb2srgb(v), 1.);
  } else {
    out_colour = vec4(rgb2oklab(v), 1.);
  }
}