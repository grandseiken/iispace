#ifndef II_GAME_RENDER_NOISE_GENERATOR_H
#define II_GAME_RENDER_NOISE_GENERATOR_H
#include "game/render/gl/texture.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace ii::render {

enum class noise_type {
  kBiome0,
};

void generate_noise_2d(gl::texture&, noise_type, const glm::uvec2& dimensions,
                       const glm::ivec2& offset);
void generate_noise_3d(gl::texture&, noise_type, const glm::uvec3& dimensions,
                       const glm::ivec3& offset);

}  // namespace ii::render

#endif