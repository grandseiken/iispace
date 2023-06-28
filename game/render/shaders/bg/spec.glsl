// Height functions.
const uint kHeightFunction_Turbulent0 = 0;

// Combinators.
const uint kCombinator_Octave0 = 0;
const uint kCombinator_Abs0 = 1;

// Tone-mapping functions.
const uint kTonemap_Tendrils = 0;
const uint kTonemap_SinCombo = 1;

struct bg_spec {
  uint height_function;
  uint combinator;
  uint tonemap;

  bool is_polar;
  float polar_period;
  // Controls how "big" the noise is; lower values mean more detail.
  float scale;
  // Controls amplitude scaling of octaves.
  float persistence;
};

// Previously: tendrils with scale = 256. Might look better without abs?
const bg_spec kBgSpec_Biome0 = {
    kHeightFunction_Turbulent0, kCombinator_Abs0, kTonemap_SinCombo,
    /* is_polar */ false,
    /* polar_period */ 0.,
    /* noise_scale */ 512.,
    /* persistence */ .25,
};

const bg_spec kBgSpec_Biome0Polar = {
    kHeightFunction_Turbulent0, kCombinator_Abs0, kTonemap_SinCombo,
    /* is_polar */ true,
    /* polar_period */ 2.,
    /* noise_scale */ 512.,
    /* persistence */ .25,
};

bg_spec get_bg_spec(uint bg_type) {
  switch (bg_type) {
  case kBgType_Biome0:
    return kBgSpec_Biome0;
  case kBgType_Biome0Polar:
    return kBgSpec_Biome0Polar;
  }
}