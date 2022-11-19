#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/shape/data.glsl"

uniform float colour_cycle;

flat in uint g_buffer_index;
out vec4 out_colour;

layout(std430, binding = 0) restrict readonly buffer shape_buffer_block {
  shape_buffer_data data[];
}
shape_buffer;

void main() {
  vec4 hsla = shape_buffer.data[g_buffer_index].colour;
  out_colour = hsla2oklab_cycle(hsla, colour_cycle);
}