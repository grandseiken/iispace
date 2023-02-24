const uint kFxStyleNone = 0;
const uint kFxStyleExplosion = 1;

const uint kFxShapeBall = 0;
const uint kFxShapeBox = 1;

struct fx_vertex_data {
  float time;
  float rotation;
  uint shape;
  uint style;
  vec4 colour;
  vec2 dimensions;
  vec2 seed;
};