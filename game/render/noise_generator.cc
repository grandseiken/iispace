#include "game/render/noise_generator.h"
#include <FastNoise/FastNoise.h>

namespace ii::render {

gl::texture generate_noise() {
  auto texture = gl::make_texture();
  return texture;
}

}  // namespace ii::render
