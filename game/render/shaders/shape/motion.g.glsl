#version 460
#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(lines) in;
layout(triangle_strip, max_vertices = 128) out;

in v_out_t {
  shape_data data;
}
v_in[];

out vec4 g_colour;

void main() {}