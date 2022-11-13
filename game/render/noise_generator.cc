#include "game/render/noise_generator.h"
#include <FastNoise/FastNoise.h>
#include <vector>

namespace ii::render {
namespace {

struct generator_data {
  const FastNoise::Generator* generator = nullptr;
  std::int32_t seed = 0;
};

std::vector<float>& noise_buffer(std::size_t size) {
  static thread_local std::vector<float> buffer;
  buffer.resize(size);
  return buffer;
}

generator_data generator_biome0() {
  static auto simplex = FastNoise::New<FastNoise::Simplex>();
  static auto fbm = FastNoise::New<FastNoise::FractalRidged>();
  static auto scale = FastNoise::New<FastNoise::DomainAxisScale>();

  fbm->SetSource(simplex);
  fbm->SetOctaveCount(2u);
  fbm->SetLacunarity(4.f);
  fbm->SetGain(.5f);

  scale->SetSource(fbm);
  scale->SetScale<FastNoise::Dim::X>(1.f / 64);
  scale->SetScale<FastNoise::Dim::Y>(1.f / 64);
  scale->SetScale<FastNoise::Dim::Z>(1.f / 128);
  return {scale.get(), 32};
}

generator_data get_generator(noise_type type) {
  switch (type) {
  case noise_type::kBiome0:
    return generator_biome0();
  }
  return {};
}

}  // namespace

gl::texture noise_texture_2d(const glm::uvec2& dimensions) {
  auto texture = gl::make_texture();
  gl::texture_storage_2d(texture, 1, dimensions, gl::internal_format::kR16F);
  return texture;
}

gl::texture noise_texture_3d(const glm::uvec3& dimensions) {
  auto texture = gl::make_texture();
  gl::texture_storage_3d(texture, 1, dimensions, gl::internal_format::kR16F);
  return texture;
}

void generate_noise_2d(gl::texture& texture, noise_type type, const glm::uvec2& dimensions,
                       const glm::ivec2& offset) {
  auto generator = get_generator(type);
  auto& buffer = noise_buffer(static_cast<std::size_t>(dimensions.x) * dimensions.y);
  generator.generator->GenUniformGrid2D(buffer.data(), offset.x, offset.y,
                                        static_cast<int>(dimensions.x),
                                        static_cast<int>(dimensions.y), 1.f, generator.seed);
  gl::texture_subimage_2d(texture, 0, glm::uvec2{0}, dimensions, gl::texture_format::kRed,
                          gl::type_of<float>(), std::span<const float>{buffer});
}

void generate_noise_3d(gl::texture& texture, noise_type type, const glm::uvec3& dimensions,
                       const glm::ivec3& offset) {
  auto generator = get_generator(type);
  auto& buffer = noise_buffer(static_cast<std::size_t>(dimensions.x) * dimensions.y * dimensions.z);
  generator.generator->GenUniformGrid3D(
      buffer.data(), offset.x, offset.y, offset.z, static_cast<int>(dimensions.x),
      static_cast<int>(dimensions.y), static_cast<int>(dimensions.z), 1.f, generator.seed);
  gl::texture_subimage_3d(texture, 0, glm::uvec3{0}, dimensions, gl::texture_format::kRed,
                          gl::type_of<float>(), std::span<const float>{buffer});
}

}  // namespace ii::render
