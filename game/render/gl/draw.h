#ifndef II_GAME_RENDER_GL_DRAW_H
#define II_GAME_RENDER_GL_DRAW_H
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <cstddef>

namespace ii::gl {

namespace clear_mask {
enum : std::uint8_t {
  kColourBufferBit = 0x1,
  kDepthBufferBit = 0x2,
  kStencilBufferBit = 0x4,
};
}  // namespace clear_mask

enum class draw_mode {
  kPoints,
  kLineStrip,
  kLineLoop,
  kLines,
  kLineStripAdjacency,
  kLinesAdjacency,
  kTriangleStrip,
  kTriangleFan,
  kTriangles,
  kTriangleStripAdjacency,
  kTrianglesAdjacency,
  kPatches,
};

enum class blend_factor {
  kZero,
  kOne,
  kSrcColour,
  kOneMinusSrcColour,
  kDstColour,
  kOneMinusDstColour,
  kSrcAlpha,
  kOneMinusSrcAlpha,
  kDstAlpha,
  kOneMinusDstAlpha,
  kConstantColour,
  kOneMinusConstantColour,
  kConstantAlpha,
  kOneMinusConstantAlpha,
  kSrcAlphaSaturate,
  kSrc1Colour,
  kOneMinusSrc1Colour,
  kSrc1Alpha,
  kOneMinusSrc1Alpha,
};

namespace detail {

inline GLenum draw_mode_to_gl(draw_mode m) {
  switch (m) {
  default:
  case draw_mode::kPoints:
    return GL_POINTS;
  case draw_mode::kLineStrip:
    return GL_LINE_STRIP;
  case draw_mode::kLineLoop:
    return GL_LINE_LOOP;
  case draw_mode::kLines:
    return GL_LINES;
  case draw_mode::kLineStripAdjacency:
    return GL_LINE_STRIP_ADJACENCY;
  case draw_mode::kLinesAdjacency:
    return GL_LINES_ADJACENCY;
  case draw_mode::kTriangleStrip:
    return GL_TRIANGLE_STRIP;
  case draw_mode::kTriangleFan:
    return GL_TRIANGLE_FAN;
  case draw_mode::kTriangles:
    return GL_TRIANGLES;
  case draw_mode::kTriangleStripAdjacency:
    return GL_TRIANGLE_STRIP_ADJACENCY;
  case draw_mode::kTrianglesAdjacency:
    return GL_TRIANGLES_ADJACENCY;
  case draw_mode::kPatches:
    return GL_PATCHES;
  }
}

inline GLenum blend_factor_to_gl(blend_factor f) {
  switch (f) {
  default:
  case blend_factor::kZero:
    return GL_ZERO;
  case blend_factor::kOne:
    return GL_ONE;
  case blend_factor::kSrcColour:
    return GL_SRC_COLOR;
  case blend_factor::kOneMinusSrcColour:
    return GL_ONE_MINUS_SRC_COLOR;
  case blend_factor::kDstColour:
    return GL_DST_COLOR;
  case blend_factor::kOneMinusDstColour:
    return GL_ONE_MINUS_DST_COLOR;
  case blend_factor::kSrcAlpha:
    return GL_SRC_ALPHA;
  case blend_factor::kOneMinusSrcAlpha:
    return GL_ONE_MINUS_SRC_ALPHA;
  case blend_factor::kDstAlpha:
    return GL_DST_ALPHA;
  case blend_factor::kOneMinusDstAlpha:
    return GL_ONE_MINUS_DST_ALPHA;
  case blend_factor::kConstantColour:
    return GL_CONSTANT_COLOR;
  case blend_factor::kOneMinusConstantColour:
    return GL_ONE_MINUS_CONSTANT_COLOR;
  case blend_factor::kConstantAlpha:
    return GL_CONSTANT_ALPHA;
  case blend_factor::kOneMinusConstantAlpha:
    return GL_ONE_MINUS_CONSTANT_ALPHA;
  case blend_factor::kSrcAlphaSaturate:
    return GL_SRC_ALPHA_SATURATE;
  case blend_factor::kSrc1Colour:
    return GL_SRC1_COLOR;
  case blend_factor::kOneMinusSrc1Colour:
    return GL_ONE_MINUS_SRC1_COLOR;
  case blend_factor::kSrc1Alpha:
    return GL_SRC1_ALPHA;
  case blend_factor::kOneMinusSrc1Alpha:
    return GL_ONE_MINUS_SRC1_ALPHA;
  }
}

}  // namespace detail

inline void clear_colour(const glm::vec4& c) {
  glClearColor(c.r, c.g, c.b, c.a);
}

inline void clear(std::uint8_t mask) {
  GLbitfield m = 0;
  if (mask & clear_mask::kColourBufferBit) {
    m |= GL_COLOR_BUFFER_BIT;
  }
  if (mask & clear_mask::kDepthBufferBit) {
    m |= GL_DEPTH_BUFFER_BIT;
  }
  if (mask & clear_mask::kStencilBufferBit) {
    m |= GL_STENCIL_BUFFER_BIT;
  }
  glClear(m);
}

inline void enable_blend(bool enable) {
  if (enable) {
    glEnable(GL_BLEND);
  } else {
    glDisable(GL_BLEND);
  }
}

inline void enable_clip_planes(std::uint32_t count) {
  for (std::uint32_t i = 0; i < 6; ++i) {
    if (i < count) {
      glEnable(GL_CLIP_DISTANCE0 + i);
    } else {
      glDisable(GL_CLIP_DISTANCE0 + i);
    }
  }
}

inline void blend_function(blend_factor source, blend_factor destination) {
  glBlendFunc(detail::blend_factor_to_gl(source), detail::blend_factor_to_gl(destination));
}

inline void draw_arrays(draw_mode mode, std::size_t first, std::size_t count) {
  glDrawArrays(detail::draw_mode_to_gl(mode), static_cast<GLint>(first),
               static_cast<GLsizei>(count));
}

inline void draw_elements(draw_mode mode, const buffer& index_buffer, type index_type,
                          std::size_t count, std::size_t offset) {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *index_buffer);
  glDrawElements(detail::draw_mode_to_gl(mode), static_cast<GLsizei>(count),
                 detail::type_to_gl(index_type), reinterpret_cast<const void*>(offset));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace ii::gl

#endif