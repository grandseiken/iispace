#include "game/render/shaders/lib/hsl.glsl"
#include "game/render/shaders/lib/rgb.glsl"

const mat3 kRgb2Lms = mat3(.4122214708, .5363325363, .0514459929, .2119034982, .6806995451,
                           .1073969566, .0883024619, .2817188376, .6299787005);
const mat3 kLms2Lab = mat3(.2104542553, .7936177850, -.0040720468, 1.9779984951, -2.4285922050,
                           .4505937099, .0259040371, .7827717662, -.8086757660);
const mat3 kLab2Lms = mat3(1., 0.3963377774, 0.2158037573, 1., -0.1055613458, -.0638541728, 1.,
                           -0.0894841775, -1.2914855480);
const mat3 kLms2Rgb = mat3(4.0767416621, -3.3077115913, .2309699292, -1.2684380046, 2.6097574011,
                           -.3413193965, -.0041960863, -.7034186147, 1.7076147010);

vec3 rgb2oklab(vec3 rgb) {
  return pow(rgb * kRgb2Lms, vec3(1. / 3)) * kLms2Lab;
}

vec3 oklab2rgb(vec3 lab) {
  vec3 lms0 = lab * kLab2Lms;
  return (lms0 * lms0 * lms0) * kLms2Rgb;
}

vec4 rgba2oklab(vec4 rgba) {
  return vec4(rgb2oklab(rgba.rgb), rgba.a);
}

vec4 oklaba2rgb(vec4 laba) {
  return vec4(oklab2rgb(laba.xyz), laba.a);
}

vec4 hsla2oklab(vec4 hsl) {
  return vec4(rgb2oklab(srgb2rgb(hsl2srgb(hsl.xyz))), hsl.a);
}

vec4 hsla2oklab_cycle(vec4 hsl, float cycle) {
  // TODO: do cycle in oklab LCh.
  return hsla2oklab(vec4(hsl.x + cycle, hsl.yz, hsl.a));
}

vec3 oklab2srgb(vec3 oklab) {
  return rgb2srgb(oklab2rgb(oklab));
}