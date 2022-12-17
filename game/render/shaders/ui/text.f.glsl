#include "game/render/shaders/lib/oklab.glsl"

uniform sampler2D font_texture;
uniform sampler2D framebuffer_texture;
uniform sampler2DMS framebuffer_texture_multisample;
uniform bool is_multisample;
uniform bool is_font_lcd;
in vec4 g_text_colour;
in vec2 g_texture_coords;

out vec4 out_colour;

void main() {
  // TODO: move as much as possible out of fragment shader; shortcut if text alpha is 1;
  // maybe discard; maybe avoid gl_SampleID.
  vec3 dst_oklab = is_multisample
      ? texelFetch(framebuffer_texture_multisample, ivec2(gl_FragCoord.xy), gl_SampleID).xyz
      : texelFetch(framebuffer_texture, ivec2(gl_FragCoord.xy), 0).xyz;
  vec3 dst_rgb = oklab2rgb(dst_oklab);

  vec3 font_rgb = texture(font_texture, g_texture_coords).rgb;
  vec3 alpha_rgb = is_font_lcd ? font_rgb : font_rgb.rrr;

  vec3 oklab = mix(dst_oklab, g_text_colour.xyz, g_text_colour.a);
  vec3 rgb = oklab2rgb(oklab);

  vec3 out_rgb = mix(dst_rgb, rgb, alpha_rgb);
  out_colour = vec4(rgb2oklab(out_rgb), 1.);
}