const uint kFxStyleNone = 0;
const uint kFxStyleExplosion = 1;

struct fx_vertex_data {
  uint style;
  vec4 colour;
  vec2 dimensions;
  vec2 seed;
};