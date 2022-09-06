const uint kPanelStyleFlatColour = 0;

struct glyph_data {
  ivec2 texture_coords;
  ivec2 dimensions;
  vec4 text_colour;
};

struct panel_data {
  ivec2 screen_dimensions;
  ivec2 render_dimensions;
  vec4 panel_colour;
  uint style;
};