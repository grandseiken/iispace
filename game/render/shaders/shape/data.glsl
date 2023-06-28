const uint kShapeStyle_NgonPolygon = 0;
const uint kShapeStyle_NgonPolystar = 1;
const uint kShapeStyle_NgonPolygram = 2;
const uint kShapeStyle_Box = 3;
const uint kShapeStyle_Line = 4;
const uint kShapeStyle_Ball = 5;

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
