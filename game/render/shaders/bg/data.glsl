const uint kBgType_None = 0;
const uint kBgType_Biome0 = 1;
const uint kBgType_Biome0Polar = 2;

struct background_data {
  ivec2 screen_dimensions;
  ivec2 render_dimensions;
  vec4 colour;
};