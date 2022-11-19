const float kPi = 3.1415926538;
const uint kStyleNgonPolygon = 0;
const uint kStyleNgonPolystar = 1;
const uint kStyleNgonPolygram = 2;
const uint kStyleBox = 3;
const uint kStyleLine = 4;

struct shape_vertex_data {
  uint buffer_index;
  uint style;
  uvec2 params;
  float rotation;
  float line_width;
  vec2 dimensions;
};

struct shape_buffer_data {
  vec4 colour;
};
