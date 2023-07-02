// Height functions.
const uint kHeightFunction_Zero = 0;
const uint kHeightFunction_Turbulent0 = 1;

// Combinators.
const uint kCombinator_Octave0 = 0;
const uint kCombinator_Abs0 = 1;

// Tone-mapping functions.
const uint kTonemap_Passthrough = 0;
const uint kTonemap_Tendrils = 1;
const uint kTonemap_SinCombo = 2;

struct bg_spec {
  uint height_function;
  uint combinator;
  uint tonemap;

  // 0 to disable.
  float polar_period;
  // Controls how "big" the noise is; lower values mean more detail.
  float scale;
  // Controls amplitude scaling of octaves.
  float persistence;

  // Controls various arbitrary things depending on functions.
  vec2 parameters;
};

struct background_data {
  ivec2 screen_dimensions;
  ivec2 render_dimensions;
  vec4 colour;
};
