const uint kPanelStyleFlatColour = 1;

struct glyph_data {
  ivec2 texture_coords;
  ivec2 dimensions;
  vec4 text_colour;
};

struct panel_data {
  ivec2 screen_dimensions;
  ivec2 render_dimensions;
  vec4 panel_colour;
  vec4 panel_border;
  uint style;
  uint border_size;
};