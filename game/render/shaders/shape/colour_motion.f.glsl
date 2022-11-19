#include "game/render/shaders/lib/oklab.glsl"
#include "game/render/shaders/shape/data.glsl"

uniform float colour_cycle;

flat in uint g_buffer_index;
in float g_interpolate;
out vec4 out_colour;

layout(std430, binding = 0) restrict readonly buffer shape_buffer_block {
  shape_buffer_data data[];
}
shape_buffer;

void main() {
  vec4 hsla0 = shape_buffer.data[g_buffer_index].colour;
  vec4 hsla1 = shape_buffer.data[g_buffer_index + 1].colour;
  vec4 oklab0 = hsla2oklab_cycle(hsla0, colour_cycle);
  vec4 oklab1 = hsla2oklab_cycle(hsla1, colour_cycle);
  out_colour = mix(vec4(oklab0.xyz, .75 * oklab0.a), vec4(oklab1.xyz, 0.), g_interpolate);
}