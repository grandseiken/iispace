#ifndef II_GAME_RENDER_GL_DATA_H
#define II_GAME_RENDER_GL_DATA_H
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ii::gl {

enum class buffer_usage {
  kStreamDraw,
  kStreamRead,
  kStreamCopy,
  kStaticDraw,
  kStaticRead,
  kStaticCopy,
  kDynamicDraw,
  kDynamicRead,
  kDynamicCopy,
};

inline buffer make_buffer() {
  id i;
  glCreateBuffers(1, &i);
  return buffer{i};
}

template <typename T>
void buffer_data(const buffer& handle, buffer_usage usage, std::span<const T> data) {
  GLenum u = 0;
  switch (usage) {
  case buffer_usage::kStreamDraw:
    u = GL_STREAM_DRAW;
    break;
  case buffer_usage::kStreamRead:
    u = GL_STREAM_READ;
    break;
  case buffer_usage::kStreamCopy:
    u = GL_STREAM_COPY;
    break;
  case buffer_usage::kStaticDraw:
    u = GL_STATIC_DRAW;
    break;
  case buffer_usage::kStaticRead:
    u = GL_STATIC_READ;
    break;
  case buffer_usage::kStaticCopy:
    u = GL_STATIC_COPY;
    break;
  case buffer_usage::kDynamicDraw:
    u = GL_DYNAMIC_DRAW;
    break;
  case buffer_usage::kDynamicRead:
    u = GL_DYNAMIC_READ;
    break;
  case buffer_usage::kDynamicCopy:
    u = GL_DYNAMIC_COPY;
    break;
  }
  auto byte_data = std::as_bytes(data);
  glNamedBufferData(*handle, byte_data.size(), byte_data.data(), u);
}

inline vertex_array make_vertex_array() {
  id i;
  glCreateVertexArrays(1, &i);
  return vertex_array{i};
}

inline void bind_vertex_array(const vertex_array& handle) {
  glBindVertexArray(*handle);
}

inline void bind_shader_storage_buffer(const buffer& handle, std::uint32_t binding) {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, *handle);
}

inline vertex_attribute vertex_float_attribute_buffer(const buffer& handle, std::uint32_t index,
                                                      std::uint8_t count_per_vertex, type data_type,
                                                      bool normalize, std::size_t stride,
                                                      std::size_t offset) {
  glEnableVertexAttribArray(index);
  glBindBuffer(GL_ARRAY_BUFFER, *handle);
  glVertexAttribPointer(index, +count_per_vertex, detail::type_to_gl(data_type),
                        normalize ? GL_TRUE : GL_FALSE, static_cast<GLsizei>(stride),
                        reinterpret_cast<const void*>(offset));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return vertex_attribute{index};
}

inline vertex_attribute vertex_int_attribute_buffer(const buffer& handle, std::uint32_t index,
                                                    std::uint8_t count_per_vertex, type data_type,
                                                    std::size_t stride, std::size_t offset) {
  glEnableVertexAttribArray(index);
  glBindBuffer(GL_ARRAY_BUFFER, *handle);
  glVertexAttribIPointer(index, count_per_vertex, detail::type_to_gl(data_type),
                         static_cast<GLsizei>(stride), reinterpret_cast<const void*>(offset));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return vertex_attribute{index};
}

inline vertex_attribute vertex_double_attribute_buffer(const buffer& handle, std::uint32_t index,
                                                       std::uint8_t count_per_vertex,
                                                       std::size_t stride, std::size_t offset) {
  glEnableVertexAttribArray(index);
  glBindBuffer(GL_ARRAY_BUFFER, *handle);
  glVertexAttribIPointer(index, count_per_vertex, GL_DOUBLE, static_cast<GLsizei>(stride),
                         reinterpret_cast<const void*>(offset));
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return vertex_attribute{index};
}

}  // namespace ii::gl

#endif