const uint kShapeStyleNgonPolygon = 0;
const uint kShapeStyleNgonPolystar = 1;
const uint kShapeStyleNgonPolygram = 2;
const uint kShapeStyleBox = 3;
const uint kShapeStyleLine = 4;
const uint kShapeStyleBall = 5;

struct shape_vertex_data {
  uint buffer_index;
  uint style;
  uvec2 params;
  float rotation;
  float line_width;
  vec2 dimensions;
};

struct shape_buffer_data {
  vec4 colour0;
  vec4 colour1;
  uint style;
  uint ball_index;
  uvec2 padding;
};

struct ball_buffer_data {
  vec2 position;
  vec2 dimensions;
  float line_width;
  float padding;
};
