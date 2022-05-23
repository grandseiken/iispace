#version 460

uniform vec4 line_colour;
out vec4 output_colour;

void main()
{
  output_colour = line_colour;
}