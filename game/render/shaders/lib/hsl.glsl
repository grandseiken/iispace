vec3 hsl2rgb(vec3 hsl) {
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

vec4 hsl2rgba(vec4 hsla) {
  return vec4(hsl2rgb(hsla.xyz), hsla.a);
}

vec4 hsl2rgba_cycle(vec4 hsla, float cycle) {
  return hsl2rgba(vec4(hsla.x + cycle, hsla.yz, hsla.a));
}
