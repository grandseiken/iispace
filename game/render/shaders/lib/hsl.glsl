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

vec3 srgb2hsl(vec3 srgb) {
  float v = max(srgb.r, max(srgb.g, srgb.b));
  float m = min(srgb.r, min(srgb.g, srgb.b));
  float l = (m + v) / 2.;
  if (l <= 0.) {
    return vec3(0.);
  }
  float vm = v - m;
  float s = vm;
  if (s <= 0.) {
    return vec3(0., 0., l);
  }
  s /= l <= .5 ? v + m : 2. - v - m;

  float r = (v - srgb.r) / vm;
  float g = (v - srgb.g) / vm;
  float b = (v - srgb.b) / vm;
  float h = 0.;
  if (srgb.r == v) {
    h = srgb.g == m ? 5. + b : 1. - g;
  } else if (srgb.g == v) {
    h = srgb.b == m ? 1. + r : 3. - b;
  } else {
    h = srgb.r == m ? 3. + g : 5. - r;
  }
  h /= 6.;
  return vec3(h, s, l);
}