uniform sampler2D framebuffer_texture;
uniform uvec2 screen_dimensions;
uniform float pixel_radius;

in vec2 v_texture_coords;
out vec4 out_colour;

void main() {
  int ir = int(ceil(pixel_radius + 1.));
  vec2 v = (1. + v_texture_coords) / 2;
  vec2 px = 1. / vec2(screen_dimensions);
  float min_sq = pixel_radius * pixel_radius;
  float max_sq = (1. + pixel_radius) * (1. + pixel_radius);

  float mask_value = 0.;
  for (int y = -ir; y <= ir; ++y) {
    for (int x = -ir; x <= ir; ++x) {
      vec2 ov = vec2(float(x), float(y));
      float q = 1. - smoothstep(min_sq, max_sq, dot(ov, ov));
      if (q > 0.) {
        vec2 tv = clamp(v + ov * px, vec2(0.), vec2(1.));
        float a = texture(framebuffer_texture, tv).a;
        mask_value = max(mask_value, q * a);
      }
    }
  }
  out_colour = vec4(1., 0., 0., min(1., mask_value));
}