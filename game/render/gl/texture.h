#ifndef II_GAME_RENDER_GL_TEXTURE_H
#define II_GAME_RENDER_GL_TEXTURE_H
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <nonstd/span.hpp>
#include <cstdint>

namespace ii::gl {
enum class internal_format {
  kDepthComponent,
  kDepthStencil,
  kRed,
  kRg,
  kRgb,
  kRgba,
  kR8,
  kR16,
  kRg8,
  kRg16,
  kR3G3B2,
  kRgb4,
  kRgb5,
  kRgb8,
  kRgb10,
  kRgb12,
  kRgb16,
  kRgba2,
  kRgba4,
  kRgb5A1,
  kRgba8,
  kRgb10A2,
  kRgba12,
  kRgba16,
  kSrgb8,
  kSrgb8Alpha8,
  // TODO: rest of these.
};

enum class texture_format {
  kRed,
  kRg,
  kRgb,
  kBgr,
  kRgba,
  kBgra,
  kRedInteger,
  kRgInteger,
  kRgbInteger,
  kBgrInteger,
  kRgbaInteger,
  kBgraInteger,
  kStencilIndex,
  kDepthComponent,
  kDepthStencil,
};

enum class filter {
  kNearest,
  kLinear,
  kNearestMipMapNearest,
  kLinearMipMapNearest,
  kNearestMipMapLinear,
  kLinearMipMapLinear,
};

enum class texture_wrap {
  kClampToEdge,
  kRepeat,
  kMirroredRepeat,
};

namespace detail {

inline GLint internal_format_to_gl(internal_format f) {
  switch (f) {
  default:
  case internal_format::kDepthComponent:
    return GL_DEPTH_COMPONENT;
  case internal_format::kDepthStencil:
    return GL_DEPTH_STENCIL;
  case internal_format::kRed:
    return GL_RED;
  case internal_format::kRg:
    return GL_RG;
  case internal_format::kRgb:
    return GL_RGB;
  case internal_format::kRgba:
    return GL_RGBA;
  case internal_format::kR8:
    return GL_R8;
  case internal_format::kR16:
    return GL_R16;
  case internal_format::kRg8:
    return GL_RG8;
  case internal_format::kRg16:
    return GL_RG16;
  case internal_format::kR3G3B2:
    return GL_R3_G3_B2;
  case internal_format::kRgb4:
    return GL_RGB4;
  case internal_format::kRgb5:
    return GL_RGB5;
  case internal_format::kRgb8:
    return GL_RGB8;
  case internal_format::kRgb10:
    return GL_RGB10;
  case internal_format::kRgb12:
    return GL_RGB12;
  case internal_format::kRgb16:
    return GL_RGB16;
  case internal_format::kRgba2:
    return GL_RGBA2;
  case internal_format::kRgba4:
    return GL_RGBA4;
  case internal_format::kRgb5A1:
    return GL_RGB5_A1;
  case internal_format::kRgba8:
    return GL_RGBA8;
  case internal_format::kRgb10A2:
    return GL_RGB10_A2;
  case internal_format::kRgba12:
    return GL_RGBA12;
  case internal_format::kRgba16:
    return GL_RGBA16;
  case internal_format::kSrgb8:
    return GL_SRGB8;
  case internal_format::kSrgb8Alpha8:
    return GL_SRGB8_ALPHA8;
  }
}

inline GLenum texture_format_to_gl(texture_format f) {
  switch (f) {
  default:
  case texture_format::kRed:
    return GL_RED;
  case texture_format::kRg:
    return GL_RG;
  case texture_format::kRgb:
    return GL_RGB;
  case texture_format::kBgr:
    return GL_BGR;
  case texture_format::kRgba:
    return GL_RGBA;
  case texture_format::kBgra:
    return GL_BGRA;
  case texture_format::kRedInteger:
    return GL_RED_INTEGER;
  case texture_format::kRgInteger:
    return GL_RG_INTEGER;
  case texture_format::kRgbInteger:
    return GL_RGB_INTEGER;
  case texture_format::kBgrInteger:
    return GL_BGR_INTEGER;
  case texture_format::kRgbaInteger:
    return GL_RGBA_INTEGER;
  case texture_format::kBgraInteger:
    return GL_BGRA_INTEGER;
  case texture_format::kStencilIndex:
    return GL_STENCIL_INDEX;
  case texture_format::kDepthComponent:
    return GL_DEPTH_COMPONENT;
  case texture_format::kDepthStencil:
    return GL_DEPTH_STENCIL;
  }
}

}  // namespace detail

inline texture make_texture() {
  id i;
  glGenTextures(1, &i);
  return texture{i};
}

inline void generate_texture_mipmap(const texture& handle) {
  glGenerateTextureMipmap(*handle);
}

template <typename T>
void texture_image_1d(const texture& handle, std::uint8_t level, internal_format iformat,
                      std::uint32_t width, texture_format format, type data_type,
                      nonstd::span<const T> data) {
  glBindTexture(GL_TEXTURE_1D, *handle);
  glTexImage1D(GL_TEXTURE_1D, static_cast<GLint>(level), detail::internal_format_to_gl(iformat),
               static_cast<GLsizei>(width), 0, detail::texture_format_to_gl(format),
               detail::type_to_gl(data_type), nonstd::as_bytes(data).data());
  glBindTexture(GL_TEXTURE_1D, 0);
}

template <typename T>
void texture_image_2d(const texture& handle, std::uint8_t level, internal_format iformat,
                      const glm::uvec2& dimensions, texture_format format, type data_type,
                      nonstd::span<const T> data) {
  glBindTexture(GL_TEXTURE_2D, *handle);
  glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), detail::internal_format_to_gl(iformat),
               dimensions.x, dimensions.y, 0, detail::texture_format_to_gl(format),
               detail::type_to_gl(data_type), nonstd::as_bytes(data).data());
  glBindTexture(GL_TEXTURE_2D, 0);
}

inline sampler
make_sampler(filter mag_filter, filter min_filter, texture_wrap wrap_s, texture_wrap wrap_t) {
  auto filter_to_gl = [](filter f) -> GLint {
    switch (f) {
    default:
    case filter::kNearest:
      return GL_NEAREST;
    case filter::kLinear:
      return GL_LINEAR;
    case filter::kNearestMipMapNearest:
      return GL_NEAREST_MIPMAP_NEAREST;
    case filter::kLinearMipMapNearest:
      return GL_LINEAR_MIPMAP_NEAREST;
    case filter::kNearestMipMapLinear:
      return GL_NEAREST_MIPMAP_LINEAR;
    case filter::kLinearMipMapLinear:
      return GL_LINEAR_MIPMAP_LINEAR;
    }
  };
  auto texture_wrap_to_gl = [](texture_wrap w) -> GLint {
    switch (w) {
    default:
    case texture_wrap::kClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case texture_wrap::kRepeat:
      return GL_REPEAT;
    case texture_wrap::kMirroredRepeat:
      return GL_MIRRORED_REPEAT;
    }
  };
  id i;
  glGenSamplers(1, &i);
  glSamplerParameteri(i, GL_TEXTURE_MAG_FILTER, filter_to_gl(mag_filter));
  glSamplerParameteri(i, GL_TEXTURE_MIN_FILTER, filter_to_gl(min_filter));
  glSamplerParameteri(i, GL_TEXTURE_WRAP_S, texture_wrap_to_gl(wrap_s));
  glSamplerParameteri(i, GL_TEXTURE_WRAP_T, texture_wrap_to_gl(wrap_t));
  return sampler{i};
}

}  // namespace ii::gl

#endif