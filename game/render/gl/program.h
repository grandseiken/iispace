#ifndef II_GAME_RENDER_GL_PROGRAM_H
#define II_GAME_RENDER_GL_PROGRAM_H
#include "game/common/result.h"
#include "game/render/gl/types.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace ii::gl {
namespace detail {
inline result<program> link_program(std::span<id> shaders) {
  program handle{glCreateProgram()};
  for (auto shader_id : shaders) {
    glAttachShader(*handle, shader_id);
  }

  glLinkProgram(*handle);
  GLint success = 0;
  glGetProgramiv(*handle, GL_LINK_STATUS, &success);

  if (!success) {
    GLint log_size = 0;
    glGetProgramiv(*handle, GL_INFO_LOG_LENGTH, &log_size);

    std::string error_log;
    error_log.resize(log_size);
    glGetProgramInfoLog(*handle, log_size, &log_size, error_log.data());
    for (auto shader_id : shaders) {
      glDetachShader(*handle, shader_id);
    }
    return unexpected(std::move(error_log));
  }

  for (auto shader_id : shaders) {
    glDetachShader(*handle, shader_id);
  }
  return handle;
}
}  // namespace detail

enum class shader_type {
  kCompute,
  kVertex,
  kTessControl,
  kTessEvaluation,
  kGeometry,
  kFragment,
};

inline result<shader> compile_shader(shader_type type, std::span<const std::uint8_t> source) {
  GLenum t = 0;
  switch (type) {
  case shader_type::kCompute:
    t = GL_COMPUTE_SHADER;
    break;
  case shader_type::kVertex:
    t = GL_VERTEX_SHADER;
    break;
  case shader_type::kTessControl:
    t = GL_TESS_CONTROL_SHADER;
    break;
  case shader_type::kTessEvaluation:
    t = GL_TESS_EVALUATION_SHADER;
    break;
  case shader_type::kGeometry:
    t = GL_GEOMETRY_SHADER;
    break;
  case shader_type::kFragment:
    t = GL_FRAGMENT_SHADER;
    break;
  default:
    return unexpected("unknown shader type");
  }

  shader handle{glCreateShader(t)};
  auto* d = reinterpret_cast<const char*>(source.data());
  auto length = static_cast<GLint>(source.size());
  glShaderSource(*handle, 1, &d, &length);

  glCompileShader(*handle);
  GLint success = 0;
  glGetShaderiv(*handle, GL_COMPILE_STATUS, &success);

  if (success) {
    return handle;
  }
  GLint log_size = 0;
  glGetShaderiv(*handle, GL_INFO_LOG_LENGTH, &log_size);

  std::string error_log;
  error_log.resize(log_size);
  glGetShaderInfoLog(*handle, log_size, &log_size, error_log.data());
  return unexpected(std::move(error_log));
}

template <typename... T>
std::enable_if_t<std::conjunction_v<std::is_same<T, shader>...>, result<program>>
link_program(const T&... shaders) {
  id ids[sizeof...(shaders)] = {*shaders...};
  return detail::link_program(ids);
}

inline result<program> link_program(std::span<shader> shaders) {
  std::vector<id> ids;
  for (auto& s : shaders) {
    ids.emplace_back(*s);
  }
  return detail::link_program(ids);
}

inline void use_program(const program& handle) {
  glUseProgram(*handle);
}

inline result<id> uniform_location(const program& handle, const char* name) {
  auto location = glGetUniformLocation(*handle, name);
  if (location < 0) {
    return unexpected("uniform '" + std::string{name} + "' not found");
  }
  return static_cast<id>(location);
}

inline result<id> uniform_location(const program& handle, const std::string& name) {
  auto location = glGetUniformLocation(*handle, name.c_str());
  if (location < 0) {
    return unexpected("uniform '" + name + "' not found");
  }
  return static_cast<id>(location);
}

inline void set_uniform(id location, float data) {
  glUniform1f(location, data);
}

inline void set_uniform(id location, int data) {
  glUniform1i(location, data);
}

inline void set_uniform(id location, unsigned int data) {
  glUniform1ui(location, data);
}

inline void set_uniform(id location, const glm::vec2& data) {
  glUniform2fv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::ivec2& data) {
  glUniform2iv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::uvec2& data) {
  glUniform2uiv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::vec3& data) {
  glUniform3fv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::ivec3& data) {
  glUniform3iv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::uvec3& data) {
  glUniform3uiv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::vec4& data) {
  glUniform4fv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::ivec4& data) {
  glUniform4iv(location, 1, glm::value_ptr(data));
}

inline void set_uniform(id location, const glm::uvec4& data) {
  glUniform4uiv(location, 1, glm::value_ptr(data));
}

inline void set_uniform_texture_1d(id location, std::uint32_t texture_unit, const texture& t_handle,
                                   const sampler& s_handle) {
  glUniform1i(location, static_cast<GLint>(texture_unit));
  glActiveTexture(GL_TEXTURE0 + texture_unit);
  glBindTexture(GL_TEXTURE_1D, *t_handle);
  glBindSampler(texture_unit, *s_handle);
}

inline void set_uniform_texture_2d(id location, std::uint32_t texture_unit, const texture& t_handle,
                                   const sampler& s_handle) {
  glUniform1i(location, static_cast<GLint>(texture_unit));
  glActiveTexture(GL_TEXTURE0 + texture_unit);
  glBindTexture(GL_TEXTURE_2D, *t_handle);
  glBindSampler(texture_unit, *s_handle);
}

inline void set_uniform_texture_3d(id location, std::uint32_t texture_unit, const texture& t_handle,
                                   const sampler& s_handle) {
  glUniform1i(location, static_cast<GLint>(texture_unit));
  glActiveTexture(GL_TEXTURE0 + texture_unit);
  glBindTexture(GL_TEXTURE_3D, *t_handle);
  glBindSampler(texture_unit, *s_handle);
}

template <typename T, typename U>
inline result<void> set_uniform(const program& handle, T&& name, U&& data) {
  auto location = uniform_location(handle, std::forward<T>(name));
  if (!location) {
    return unexpected(location.error());
  }
  set_uniform(*location, std::forward<U>(data));
  return {};
}

template <typename T>
result<void> set_uniform_texture_1d(const program& handle, T&& name, std::uint32_t texture_unit,
                                    const texture& t_handle, const sampler& s_handle) {
  auto location = uniform_location(handle, std::forward<T>(name));
  if (!location) {
    return unexpected(location.error());
  }
  set_uniform_texture_1d(*location, texture_unit, t_handle, s_handle);
  return {};
}

template <typename T>
result<void> set_uniform_texture_2d(const program& handle, T&& name, std::uint32_t texture_unit,
                                    const texture& t_handle, const sampler& s_handle) {
  auto location = uniform_location(handle, std::forward<T>(name));
  if (!location) {
    return unexpected(location.error());
  }
  set_uniform_texture_2d(*location, texture_unit, t_handle, s_handle);
  return {};
}

template <typename T>
result<void> set_uniform_texture_3d(const program& handle, T&& name, std::uint32_t texture_unit,
                                    const texture& t_handle, const sampler& s_handle) {
  auto location = uniform_location(handle, std::forward<T>(name));
  if (!location) {
    return unexpected(location.error());
  }
  set_uniform_texture_3d(*location, texture_unit, t_handle, s_handle);
  return {};
}

inline result<void> set_uniforms(const program&) {
  return {};
}

template <typename T, typename U, typename... Args>
inline result<void> set_uniforms(const program& handle, T&& name, U&& data, Args&&... args) {
  auto r = set_uniform(handle, std::forward<T>(name), std::forward<U>(data));
  if (!r) {
    return r;
  }
  return set_uniforms(handle, std::forward<Args>(args)...);
}

}  // namespace ii::gl

#endif