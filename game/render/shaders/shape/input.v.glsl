#include "game/render/shaders/shape/data.glsl"

layout(location = 0) in uint in_buffer_index;
layout(location = 1) in uint in_style;
layout(location = 2) in uvec2 in_params;
layout(location = 3) in float in_rotation;
layout(location = 4) in float in_line_width;
layout(location = 5) in vec3 in_position;
layout(location = 6) in vec2 in_dimensions;

out v_out_t {
  shape_vertex_data data;
}
v_out;

void main() {
  v_out.data.buffer_index = in_buffer_index;
  v_out.data.style = in_style;
  v_out.data.params = in_params;
  v_out.data.rotation = in_rotation;
  v_out.data.line_width = in_line_width;
  v_out.data.dimensions = in_dimensions;
  gl_Position = vec4(in_position, 1.);
}