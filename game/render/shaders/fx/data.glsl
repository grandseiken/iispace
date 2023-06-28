const uint kFxStyle_None = 0;
const uint kFxStyle_Explosion = 1;

const uint kFxShape_Ball = 0;
const uint kFxShape_Box = 1;

struct fx_vertex_data {
  float time;
  float rotation;
  uint shape;
  uint style;
  vec4 colour;
  vec2 dimensions;
  vec2 seed;
};