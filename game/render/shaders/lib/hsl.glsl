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

vec3 hsl2srgb(vec3 hsl) {
  float h = fract(hsl.x);
  float s = hsl.y;
  float l = hsl.z;
  float v = l <= .5 ? l * (1. + s) : l + s - l * s;

  if (v == 0.) {
    return vec3(0.);
  }
  float m = 2. * l - v;
  float sv = (v - m) / v;
  float sextant = h * 6.;
  float vsf = v * sv * fract(h * 6.);
  float mid1 = m + vsf;
  float mid2 = v - vsf;

  if (sextant < 1.) {
    return vec3(v, mid1, m);
  } else if (sextant < 2.) {
    return vec3(mid2, v, m);
  } else if (sextant < 3.) {
    return vec3(m, v, mid1);
  } else if (sextant < 4.) {
    return vec3(m, mid2, v);
  } else if (sextant < 5.) {
    return vec3(mid1, m, v);
  } else {
    return vec3(v, m, mid2);
  }
}

vec3 hsl2rgb(vec3 hsl) {
  return srgb2rgb(hsl2srgb(hsl));
}

vec4 hsl2rgba(vec4 hsla) {
  return vec4(hsl2rgb(hsla.xyz), hsla.a);
}

vec4 hsl2rgba_cycle(vec4 hsla, float cycle) {
  return hsl2rgba(vec4(hsla.x + cycle, hsla.yz, hsla.a));
}
