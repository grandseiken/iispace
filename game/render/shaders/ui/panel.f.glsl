#include "game/render/shaders/ui/data.glsl"

flat in vec2 g_render_dimensions;
flat in vec2 g_screen_dimensions;
flat in vec4 g_panel_colour;
flat in vec4 g_panel_border;
flat in uint g_style;
flat in uint g_border_size;
in vec2 g_texture_coords;
out vec4 out_colour;

void main() {
  vec2 screen_coords = g_texture_coords * g_screen_dimensions;
  bool is_border = screen_coords.x < g_border_size ||
      screen_coords.x > g_screen_dimensions.x - g_border_size || screen_coords.y < g_border_size ||
      screen_coords.y > g_screen_dimensions.y - g_border_size;
  switch (g_style) {
  case kPanelStyleFlatColour:
    if (is_border && g_panel_border.a > 0) {
      out_colour = g_panel_border;
    } else {
      out_colour = g_panel_colour;
    }
    break;
  }
}