#ifndef II_GAME_RENDER_NOISE_GENERATOR_H
#define II_GAME_RENDER_NOISE_GENERATOR_H
#include "game/render/gl/texture.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace ii::render {

enum class noise_type {
  kBiome0,
};

gl::texture generate_noise_2d(noise_type, const glm::uvec2& dimensions, const glm::ivec2& offset);
gl::texture generate_noise_3d(noise_type, const glm::uvec3& dimensions, const glm::ivec3& offset);

}  // namespace ii::render

#endif