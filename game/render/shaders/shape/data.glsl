const float kPi = 3.1415926538;
const uint kStyleNgonPolygon = 0;
const uint kStyleNgonPolystar = 1;
const uint kStyleNgonPolygram = 2;
const uint kStyleBox = 3;
const uint kStyleLine = 4;

struct shape_data {
  uint style;
  uvec2 params;
  float rotation;
  float line_width;
  vec2 dimensions;
  vec4 colour;
};
