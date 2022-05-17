#include "game/io/sdl_io.h"
#include "external/sdl_gamecontrollerdb/gamecontrollerdb.txt.h"
#include "game/io/sdl_convert.h"
#include <GL/gl3w.h>
#include <SDL.h>
#include <unordered_map>

namespace ii::io {

struct SdlIoLayer::impl_t {
  std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> window{nullptr, nullptr};
  std::unique_ptr<void, void (*)(void*)> gl_context{nullptr, nullptr};

  struct controller_data {
    std::unique_ptr<SDL_GameController, void (*)(SDL_GameController*)> controller{nullptr, nullptr};
    controller::frame frame = {};
  };
  std::vector<controller_data> controllers;
  std::unordered_map<SDL_JoystickID, std::size_t> controller_map;
  mouse::frame mouse_frame;
  keyboard::frame keyboard_frame;

  void scan_controllers() {
    controllers.clear();
    controller_map.clear();
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      if (!SDL_IsGameController(i)) {
        continue;
      }
      auto* controller = SDL_GameControllerOpen(i);
      if (!controller) {
        continue;
      }
      std::unique_ptr<SDL_GameController, void (*)(SDL_GameController*)> unique_controller{
          controller, &SDL_GameControllerClose};

      auto* joystick = SDL_GameControllerGetJoystick(controller);
      if (!joystick) {
        continue;
      }

      auto id = SDL_JoystickGetDeviceInstanceID(i);
      controller_map.emplace(id, controllers.size());
      auto& data = controllers.emplace_back();
      data.controller = std::move(unique_controller);
    }
  }

  controller_data* controller(SDL_JoystickID id) {
    if (auto it = controller_map.find(id); it != controller_map.end()) {
      return &controllers[it->second];
    }
    return nullptr;
  }
};

result<std::unique_ptr<SdlIoLayer>>
SdlIoLayer::create(const char* title, char gl_major, char gl_minor) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER |
               SDL_INIT_HAPTIC) < 0) {
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

  auto* rwops = SDL_RWFromConstMem(external_sdl_gamecontrollerdb_gamecontrollerdb_txt,
                                   external_sdl_gamecontrollerdb_gamecontrollerdb_txt_len);
  if (!rwops) {
    return unexpected("couldn't read SDL_GameControllerDB");
  }
  std::unique_ptr<SDL_RWops, int (*)(SDL_RWops*)> unique_rwops{rwops, &SDL_RWclose};
  if (SDL_GameControllerAddMappingsFromRW(rwops, /* no close */ 0) < 0) {
    return unexpected(SDL_GetError());
  }

  io_layer->impl_->scan_controllers();
  return io_layer;
}

SdlIoLayer::SdlIoLayer(access_tag) {}

SdlIoLayer::~SdlIoLayer() {
  if (impl_) {
    impl_.reset();
    SDL_Quit();
  }
}

void SdlIoLayer::swap_buffers() {
  SDL_GL_SwapWindow(impl_->window.get());
}

std::optional<event_type> SdlIoLayer::poll() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
        return event_type::kClose;
      }
      break;

    case SDL_JOYDEVICEADDED:
    case SDL_JOYDEVICEREMOVED:
      impl_->scan_controllers();
      return event_type::kControllerChange;

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
      {
        auto* data = impl_->controller(event.cbutton.which);
        if (auto button = convert_sdl_controller_button(event.cbutton.button); data && button) {
          auto& e = data->frame.button_events.emplace_back();
          e.button = *button;
          e.down = event.type == SDL_CONTROLLERBUTTONDOWN;
          data->frame.button(*button) = e.down;
        }
      }
      break;

    case SDL_CONTROLLERAXISMOTION:
      {
        auto* data = impl_->controller(event.caxis.which);
        if (auto axis = convert_sdl_controller_axis(event.caxis.axis); data && axis) {
          data->frame.axis(*axis) = event.caxis.value;
        }
      }
      break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      if (auto key = convert_sdl_key(event.key.keysym.sym); key) {
        auto& e = impl_->keyboard_frame.key_events.emplace_back();
        e.key = *key;
        e.down = event.type == SDL_KEYDOWN;
        impl_->keyboard_frame.key(*key) = e.down;
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      if (auto button = convert_sdl_mouse_button(event.button.button); button) {
        auto& e = impl_->mouse_frame.button_events.emplace_back();
        e.button = *button;
        e.down = event.type == SDL_MOUSEBUTTONDOWN;
        impl_->mouse_frame.button(*button) = e.down;
      }
      break;

    case SDL_MOUSEMOTION:
      impl_->mouse_frame.cursor = {event.motion.x, event.motion.y};
      impl_->mouse_frame.cursor_delta += glm::ivec2{event.motion.xrel, event.motion.yrel};
      break;

    case SDL_MOUSEWHEEL:
      if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
        impl_->mouse_frame.wheel_delta -= glm::ivec2{event.wheel.x, event.wheel.y};
      } else {
        impl_->mouse_frame.wheel_delta += glm::ivec2{event.wheel.x, event.wheel.y};
      }
      break;
    }
  }
  return std::nullopt;
}

std::vector<controller::info> SdlIoLayer::controller_info() const {
  std::vector<controller::info> result;
  for (const auto& data : impl_->controllers) {
    auto& info = result.emplace_back();
    if (auto* name = SDL_GameControllerName(data.controller.get()); name) {
      info.name = name;
    } else {
      info.name = "Unknown game controller";
    }
    switch (SDL_GameControllerGetType(data.controller.get())) {
    case SDL_CONTROLLER_TYPE_PS3:
    case SDL_CONTROLLER_TYPE_PS4:
    case SDL_CONTROLLER_TYPE_PS5:
      info.type = controller::type::kPsx;
      break;
    case SDL_CONTROLLER_TYPE_XBOX360:
    case SDL_CONTROLLER_TYPE_XBOXONE:
      info.type = controller::type::kXbox;
      break;
    default:
      info.type = controller::type::kOther;
      break;
    }
    auto player_index = SDL_GameControllerGetPlayerIndex(data.controller.get());
    if (player_index >= 0) {
      info.player_index = static_cast<std::uint32_t>(player_index);
    }
  }
  return result;
}

controller::frame SdlIoLayer::controller_frame(std::size_t index) const {
  if (index >= impl_->controllers.size()) {
    return {};
  }
  return impl_->controllers[index].frame;
}

keyboard::frame SdlIoLayer::keyboard_frame() const {
  return impl_->keyboard_frame;
}

mouse::frame SdlIoLayer::mouse_frame() const {
  return impl_->mouse_frame;
}

void SdlIoLayer::input_frame_clear() {
  for (auto& data : impl_->controllers) {
    data.frame.button_events.clear();
  }
  impl_->keyboard_frame.key_events.clear();
  impl_->mouse_frame.button_events.clear();
  impl_->mouse_frame.cursor_delta = {0, 0};
  impl_->mouse_frame.wheel_delta = {0, 0};
}

}  // namespace ii::io