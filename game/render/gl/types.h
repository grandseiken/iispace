#ifndef IISPACE_GAME_RENDER_GL_TYPES_H
#define IISPACE_GAME_RENDER_GL_TYPES_H
#include "game/common/result.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <type_traits>

namespace ii::gl {
using id = std::uint32_t;

enum class type {
  kByte,
  kUnsignedByte,
  kShort,
  kUnsignedShort,
  kInt,
  kUnsignedInt,
  kHalfFloat,
  kFloat,
  kDouble,
  kFixed,
};

template <typename T>
type type_of() {
  using t = std::remove_cv_t<std::remove_reference_t<T>>;
  if constexpr (std::is_same_v<t, std::int8_t>) {
    return type::kByte;
  } else if constexpr (std::is_same_v<t, std::uint8_t> || std::is_same_v<t, std::byte>) {
    return type::kUnsignedByte;
  } else if constexpr (std::is_same_v<t, short>) {
    return type::kShort;
  } else if constexpr (std::is_same_v<t, unsigned short>) {
    return type::kUnsignedShort;
  } else if constexpr (std::is_same_v<t, int>) {
    return type::kInt;
  } else if constexpr (std::is_same_v<t, unsigned int>) {
    return type::kUnsignedInt;
  } else if constexpr (std::is_same_v<t, float>) {
    return type::kFloat;
  } else if constexpr (std::is_same_v<t, double>) {
    return type::kDouble;
  } else {
    static_assert(false, "unsupported type");
  }
}

namespace detail {

template <typename D>
class handle {
public:
  explicit handle(id i) : id_{i} {}
  handle(const handle&) = delete;
  handle& operator=(const handle&) = delete;

  handle(handle&& h) : id_(h.id_) {
    h.id_.reset();
  }

  handle& operator=(handle&& h) {
    if (this == &h) {
      return *this;
    }
    if (id_) {
      D{}(*id_);
    }
    id_ = h.id_;
    h.id_.reset();
    return *this;
  }

  id operator*() const {
    return *id_;
  }

  ~handle() {
    if (id_) {
      D{}(*id_);
      id_.reset();
    }
  }

private:
  std::optional<id> id_;
};

struct delete_shader {
  void operator()(id i) const {
    glDeleteShader(i);
  }
};
struct delete_program {
  void operator()(id i) const {
    glDeleteProgram(i);
  }
};
struct delete_buffer {
  void operator()(id i) const {
    glDeleteBuffers(1, &i);
  }
};
struct delete_texture {
  void operator()(id i) const {
    glDeleteTextures(1, &i);
  }
};
struct delete_sampler {
  void operator()(id i) const {
    glDeleteSamplers(1, &i);
  }
};
struct delete_vertex_array {
  void operator()(id i) const {
    glDeleteVertexArrays(1, &i);
  }
};
struct disable_vertex_attribute {
  void operator()(id i) const {
    glDisableVertexAttribArray(i);
  }
};

inline GLenum type_to_gl(type t) {
  switch (t) {
  default:
  case type::kByte:
    return GL_BYTE;
  case type::kUnsignedByte:
    return GL_UNSIGNED_BYTE;
  case type::kShort:
    return GL_SHORT;
  case type::kUnsignedShort:
    return GL_UNSIGNED_SHORT;
  case type::kInt:
    return GL_INT;
  case type::kUnsignedInt:
    return GL_UNSIGNED_INT;
  case type::kHalfFloat:
    return GL_HALF_FLOAT;
  case type::kFloat:
    return GL_FLOAT;
  case type::kDouble:
    return GL_DOUBLE;
  case type::kFixed:
    return GL_FIXED;
  }
};

}  // namespace detail

using shader = detail::handle<detail::delete_shader>;
using program = detail::handle<detail::delete_program>;
using buffer = detail::handle<detail::delete_buffer>;
using texture = detail::handle<detail::delete_texture>;
using sampler = detail::handle<detail::delete_sampler>;
using vertex_array = detail::handle<detail::delete_vertex_array>;
using vertex_attribute = detail::handle<detail::disable_vertex_attribute>;

inline result<void> check_error() {
  auto error = glGetError();
  if (error == GL_NO_ERROR) {
    return {};
  }
  switch (error) {
  case GL_NO_ERROR:
    return {};
  case GL_INVALID_ENUM:
    return unexpected("invalid enum");
  case GL_INVALID_VALUE:
    return unexpected("invalid value");
  case GL_INVALID_OPERATION:
    return unexpected("invalid operation");
  case GL_INVALID_FRAMEBUFFER_OPERATION:
    return unexpected("invalid framebuffer operation");
  case GL_OUT_OF_MEMORY:
    return unexpected("out of memory");
  default:
    return unexpected("unknown error");
  }
}

}  // namespace ii::gl

#endif