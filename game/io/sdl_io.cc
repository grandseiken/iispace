#include "game/io/sdl_io.h"
#include <GL/gl3w.h>
#include <SDL.h>
#include <unordered_map>

namespace ii::io {
namespace {

std::optional<controller::axis> convert_controller_axis(SDL_GameControllerAxis axis) {
  switch (axis) {
  case SDL_CONTROLLER_AXIS_LEFTX:
    return controller::axis::kLX;
  case SDL_CONTROLLER_AXIS_LEFTY:
    return controller::axis::kLY;
  case SDL_CONTROLLER_AXIS_RIGHTX:
    return controller::axis::kRX;
  case SDL_CONTROLLER_AXIS_RIGHTY:
    return controller::axis::kRY;
  case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
    return controller::axis::kLT;
  case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
    return controller::axis::kRT;
  default:
    return std::nullopt;
  }
}

std::optional<controller::button> convert_controller_button(SDL_GameControllerButton button) {
  switch (button) {
  case SDL_CONTROLLER_BUTTON_A:
    return controller::button::kA;
  case SDL_CONTROLLER_BUTTON_B:
    return controller::button::kB;
  case SDL_CONTROLLER_BUTTON_X:
    return controller::button::kX;
  case SDL_CONTROLLER_BUTTON_Y:
    return controller::button::kY;
  case SDL_CONTROLLER_BUTTON_BACK:
    return controller::button::kBack;
  case SDL_CONTROLLER_BUTTON_GUIDE:
    return controller::button::kGuide;
  case SDL_CONTROLLER_BUTTON_START:
    return controller::button::kStart;
  case SDL_CONTROLLER_BUTTON_LEFTSTICK:
    return controller::button::kLStick;
  case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
    return controller::button::kRStick;
  case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
    return controller::button::kLShoulder;
  case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
    return controller::button::kRShoulder;
  case SDL_CONTROLLER_BUTTON_DPAD_UP:
    return controller::button::kDpadUp;
  case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
    return controller::button::kDpadDown;
  case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
    return controller::button::kDpadLeft;
  case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
    return controller::button::kDpadRight;
  case SDL_CONTROLLER_BUTTON_MISC1:
    return controller::button::kMisc1;
  case SDL_CONTROLLER_BUTTON_PADDLE1:
    return controller::button::kPaddle1;
  case SDL_CONTROLLER_BUTTON_PADDLE2:
    return controller::button::kPaddle2;
  case SDL_CONTROLLER_BUTTON_PADDLE3:
    return controller::button::kPaddle3;
  case SDL_CONTROLLER_BUTTON_PADDLE4:
    return controller::button::kPaddle4;
  case SDL_CONTROLLER_BUTTON_TOUCHPAD:
    return controller::button::kTouchpad;
  default:
    return std::nullopt;
  }
}

}  // namespace

struct SdlIoLayer::impl_t {
  std::unique_ptr<SDL_Window, void (*)(SDL_Window*)> window{nullptr, nullptr};
  std::unique_ptr<void, void (*)(void*)> gl_context{nullptr, nullptr};

  struct controller_data {
    std::unique_ptr<SDL_GameController, void (*)(SDL_GameController*)> controller{nullptr, nullptr};
    controller::frame frame = {};
  };
  std::vector<controller_data> controllers;
  std::unordered_map<SDL_JoystickID, std::size_t> controller_map;

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
        auto button =
            convert_controller_button(static_cast<SDL_GameControllerButton>(event.cbutton.button));
        if (data && button) {
          data->frame.button_state[static_cast<std::size_t>(*button)] =
              event.type == SDL_CONTROLLERBUTTONDOWN;
        }
      }
      break;

    case SDL_CONTROLLERAXISMOTION:
      {
        auto* data = impl_->controller(event.caxis.which);
        auto axis = convert_controller_axis(static_cast<SDL_GameControllerAxis>(event.caxis.axis));
        if (data && axis) {
          data->frame.axis_state[static_cast<std::size_t>(*axis)] = event.caxis.value;
        }
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

void SdlIoLayer::input_frame_clear() {
  for (auto& data : impl_->controllers) {
    // TODO: clear button events.
  }
}

}  // namespace ii::io