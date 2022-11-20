const float kPi = 3.1415926538;
const uint kStyleNgonPolygon = 0;
const uint kStyleNgonPolystar = 1;
const uint kStyleNgonPolygram = 2;
const uint kStyleBox = 3;
const uint kStyleLine = 4;
const uint kStyleBall = 5;

struct shape_vertex_data {
  uint buffer_index;
  uint style;
  uvec2 params;
  float rotation;
  float line_width;
  vec2 dimensions;
};

// TODO: most of this not needed for most shapes; should split
// into separate buffer (with index here).
struct shape_buffer_data {
  uvec4 u_params;
  vec4 f_params;
  vec4 colour;
  vec2 position;
  vec2 dimensions;
};
