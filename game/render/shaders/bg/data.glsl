const uint kBgTypeNone = 0;
const uint kBgTypeBiome0 = 1;
const uint kBgTypeBiome0_Polar = 2;

struct background_data {
  ivec2 screen_dimensions;
  ivec2 render_dimensions;
  vec4 colour;
};