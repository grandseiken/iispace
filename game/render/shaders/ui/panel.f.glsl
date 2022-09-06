#include "game/render/shaders/ui/data.glsl"

flat in vec2 g_render_dimensions;
flat in vec4 g_panel_colour;
flat in uint g_style;
in vec2 g_texture_coords;
out vec4 out_colour;

void main() {
  switch (g_style) {
  case kPanelStyleFlatColour:
    out_colour = g_panel_colour;
    break;
  }
}