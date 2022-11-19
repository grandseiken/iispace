float srgb2rgb_component(float v) {
  return v >= .04045 ? pow((v + .055) / 1.055, 2.4) : v / 12.92;
}

float rgb2srgb_component(float v) {
  return v >= .0031308 ? 1.055 * pow(v, 1. / 2.4) - .055 : v * 12.92;
}

vec3 srgb2rgb(vec3 srgb) {
  return vec3(srgb2rgb_component(srgb.r), srgb2rgb_component(srgb.g), srgb2rgb_component(srgb.b));
}

vec3 rgb2srgb(vec3 rgb) {
  return vec3(rgb2srgb_component(rgb.r), rgb2srgb_component(rgb.g), rgb2srgb_component(rgb.b));
}