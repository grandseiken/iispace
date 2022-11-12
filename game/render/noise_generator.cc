#include "game/render/noise_generator.h"
#include <FastNoise/FastNoise.h>
#include <vector>

namespace ii::render {
namespace {

struct generator_data {
  const FastNoise::Generator* generator = nullptr;
  float frequency = 1.f;
  std::int32_t seed = 0;
};

std::vector<float>& noise_buffer(std::size_t size) {
  static thread_local std::vector<float> buffer;
  buffer.resize(size);
  return buffer;
}

generator_data generator_biome0() {
  static auto simplex = FastNoise::New<FastNoise::Simplex>();
  static auto fbm = FastNoise::New<FastNoise::FractalFBm>();

  fbm->SetSource(simplex);
  fbm->SetOctaveCount(8u);
  fbm->SetLacunarity(2.f);
  fbm->SetGain(.5f);
  return {fbm.get(), 1.f, 1};
}

generator_data get_generator(noise_type type) {
  switch (type) {
  case noise_type::kBiome0:
    return generator_biome0();
  }
  return {};
}

}  // namespace

gl::texture
generate_noise_2d(noise_type type, const glm::uvec2& dimensions, const glm::ivec2& offset) {
  auto generator = get_generator(type);
  auto& buffer = noise_buffer(static_cast<std::size_t>(dimensions.x) * dimensions.y);
  generator.generator->GenUniformGrid2D(
      buffer.data(), offset.x, offset.y, static_cast<int>(dimensions.x),
      static_cast<int>(dimensions.y), generator.frequency, generator.seed);

  auto texture = gl::make_texture();
  gl::texture_image_2d(texture, 0, gl::internal_format::kRed, dimensions, gl::texture_format::kRed,
                       gl::type_of<float>(), std::span<const float>{buffer});
  return texture;
}

gl::texture
generate_noise_3d(noise_type type, const glm::uvec3& dimensions, const glm::ivec3& offset) {
  auto generator = get_generator(type);
  auto& buffer = noise_buffer(static_cast<std::size_t>(dimensions.x) * dimensions.y * dimensions.z);
  generator.generator->GenUniformGrid3D(
      buffer.data(), offset.x, offset.y, offset.z, static_cast<int>(dimensions.x),
      static_cast<int>(dimensions.y), static_cast<int>(dimensions.z), generator.frequency,
      generator.seed);

  auto texture = gl::make_texture();
  gl::texture_image_3d(texture, 0, gl::internal_format::kRed, dimensions, gl::texture_format::kRed,
                       gl::type_of<float>(), std::span<const float>{buffer});
  return texture;
}

}  // namespace ii::render
