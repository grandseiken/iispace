vec3 rgb2oklab(vec3 rgb) {
  vec3 lms = vec3(.4122214708 * rgb.r + .5363325363 * rgb.g + .0514459929 * rgb.b,
                  .2119034982 * rgb.r + .6806995451 * rgb.g + .1073969566 * rgb.b,
                  .0883024619 * rgb.r + .2817188376 * rgb.g + .6299787005 * rgb.b);
  vec3 lms0 = pow(lms, vec3(1. / 3));
  return vec3(.2104542553 * lms0.x + .7936177850 * lms0.y - .0040720468 * lms0.z,
              1.9779984951 * lms0.x - 2.4285922050 * lms0.y + .4505937099 * lms0.z,
              .0259040371 * lms0.x + .7827717662 * lms0.y - .8086757660 * lms0.z);
}

vec3 oklab2rgb(vec3 lab) {
  vec3 lms0 = vec3(lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z,
                   lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z,
                   lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z);
  vec3 lms = lms0 * lms0 * lms0;
  return vec3(4.0767416621 * lms.x - 3.3077115913 * lms.y + .2309699292 * lms.z,
              -1.2684380046 * lms.x + 2.6097574011 * lms.y - .3413193965 * lms.z,
              -.0041960863 * lms.x - .7034186147 * lms.y + 1.7076147010 * lms.z);
}