#include "game/io/sdl_io.h"
#include <SDL.h>
#include <GL/gl3w.h>

namespace ii::io {

struct SdlIoLayer::impl_t {
  bool should_close = false;

  std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> window{nullptr, nullptr};
  std::unique_ptr<void, void (*)(void*)> gl_context{nullptr, nullptr};
};

result<std::unique_ptr<SdlIoLayer>>
SdlIoLayer::create(const char* title, char gl_major, char gl_minor) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0) {
    return unexpected(SDL_GetError());
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor);

  auto io_layer = std::make_unique<SdlIoLayer>(access_tag{});
  io_layer->impl_ = std::make_unique<impl_t>();

  auto* window = SDL_CreateWindow(
      title, 0, 0, 0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
  if (!window) {
    return unexpected(SDL_GetError());
  }
  io_layer->impl_->window =
      std::unique_ptr<SDL_Window, void (*)(SDL_Window*)>{window, &SDL_DestroyWindow};

  auto* gl_context = SDL_GL_CreateContext(window);
  if (!gl_context) {
    return unexpected(SDL_GetError());
  }
  io_layer->impl_->gl_context =
      std::unique_ptr<void, void (*)(void*)>{gl_context, &SDL_GL_DeleteContext};

  if (gl3wInit()) {
    return unexpected("failed to initialize gl3w");
  }

  if (!gl3wIsSupported(gl_major, gl_minor)) {
    return unexpected("OpenGL " + std::to_string(+gl_major) + "." + std::to_string(+gl_minor) +
                      " not supported");
  }
  return io_layer;
}

SdlIoLayer::SdlIoLayer(access_tag) {}

SdlIoLayer::~SdlIoLayer() {
  SDL_Quit();
}

void SdlIoLayer::update() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
      impl_->should_close = true;
    }
  }
}

void SdlIoLayer::swap_buffers() {
  SDL_GL_SwapWindow(impl_->window.get());
}

bool SdlIoLayer::should_close() const {
  return impl_->should_close;
}

}  // namespace ii::io