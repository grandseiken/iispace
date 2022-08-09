#include "game/io/sdl_io.h"
#include "external/sdl_gamecontrollerdb/sdl_gamecontrollerdb.h"
#include "game/common/raw_ptr.h"
#include "game/io/sdl_convert.h"
#include <GL/gl3w.h>
#include <SDL.h>
#include <sfn/functional.h>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace ii::io {

struct SdlIoLayer::impl_t {
  raw_ptr<SDL_Window> window;
  raw_ptr<void> gl_context;

  struct controller_data {
    raw_ptr<SDL_GameController> controller;
    controller::frame frame = {};
  };
  std::vector<controller_data> controllers;
  std::unordered_map<SDL_JoystickID, std::size_t> controller_map;
  mouse::frame mouse_frame;
  keyboard::frame keyboard_frame;

  SDL_AudioDeviceID audio_device_id;
  raw_ptr<SDL_AudioDeviceID> audio_device;
  std::function<io::audio_callback> audio_callback;

  result<void> open_audio_device(const std::optional<std::string>& name) {
    audio_device.reset();

    SDL_AudioSpec desired_spec;
    SDL_AudioSpec obtained_spec;
    std::memset(&desired_spec, 0, sizeof(desired_spec));
    std::memset(&obtained_spec, 0, sizeof(obtained_spec));

    desired_spec.freq = static_cast<int>(kAudioSampleRate);
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.samples = 1024;
    desired_spec.callback =
        sfn::reinterpret<void(void*, std::uint8_t*, int), &impl_t::audio_callback_function>;
    desired_spec.userdata = this;
    auto id = SDL_OpenAudioDevice(name ? name->c_str() : nullptr, /* capture */ 0, &desired_spec,
                                  &obtained_spec, /* allowed spec changes */ 0);
    if (!id) {
      return unexpected(SDL_GetError());
    }

    audio_device_id = id;
    audio_device = make_raw(
        &audio_device_id, +[](SDL_AudioDeviceID* id) {
          if (*id) {
            SDL_CloseAudioDevice(*id);
            *id = 0;
          }
        });

    if (obtained_spec.freq != desired_spec.freq || obtained_spec.format != desired_spec.format ||
        obtained_spec.channels != desired_spec.channels) {
      return unexpected("audio device opened with unexpected format");
      audio_device.reset();
    }

    SDL_PauseAudioDevice(audio_device_id, /* unpause */ 0);
    return {};
  }

  void audio_callback_function(std::uint8_t* out_buffer, int size_bytes) {
    std::size_t samples = size_bytes / (2 * sizeof(audio_sample_t));
    if (audio_callback) {
      audio_callback(out_buffer, samples);
    } else {
      std::memset(out_buffer, 0, 2 * samples * sizeof(audio_sample_t));
    }
  }

  void scan_controllers() {
    controllers.clear();
    controller_map.clear();
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
      if (!SDL_IsGameController(i)) {
        continue;
      }
      auto controller = make_raw(SDL_GameControllerOpen(i), &SDL_GameControllerClose);
      if (!controller) {
        continue;
      }

      auto* joystick = SDL_GameControllerGetJoystick(controller.get());
      if (!joystick) {
        continue;
      }

      auto id = SDL_JoystickGetDeviceInstanceID(i);
      controller_map.emplace(id, controllers.size());
      auto& data = controllers.emplace_back();
      data.controller = std::move(controller);
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
  // TODO: SDL_INIT_GAMECONTROLLER can sometimes cause hangs or long delays while probing
  // misbehaving devices or buggy drivers. Apparently it's possible to run that bit asynchronously
  // on a different thread and start using gamepads once it finishes?
  // https://mobile.twitter.com/noelfb/status/1256794955227361280
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) < 0) {
    return unexpected(SDL_GetError());
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

  auto io_layer = std::make_unique<SdlIoLayer>(access_tag{});
  io_layer->impl_ = std::make_unique<impl_t>();

  auto flags =
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN_DESKTOP;
  auto window = make_raw(SDL_CreateWindow(title, 0, 0, 0, 0, flags), &SDL_DestroyWindow);
  if (!window) {
    return unexpected(SDL_GetError());
  }
  SDL_ShowCursor(SDL_DISABLE);

  auto gl_context = make_raw(SDL_GL_CreateContext(window.get()), &SDL_GL_DeleteContext);
  if (!gl_context) {
    return unexpected(SDL_GetError());
  }
  io_layer->impl_->window = std::move(window);
  io_layer->impl_->gl_context = std::move(gl_context);

  if (gl3wInit()) {
    return unexpected("failed to initialize gl3w");
  }

  if (!gl3wIsSupported(gl_major, gl_minor)) {
    return unexpected("OpenGL " + std::to_string(+gl_major) + "." + std::to_string(+gl_minor) +
                      " not supported");
  }
  SDL_GL_SetSwapInterval(1);

  auto rwops = make_raw(
      SDL_RWFromConstMem(gamecontrollerdb_txt().data(), gamecontrollerdb_txt().size()),
      +[](SDL_RWops* p) { SDL_RWclose(p); });
  if (!rwops) {
    return unexpected("couldn't read SDL_GameControllerDB");
  }
  if (SDL_GameControllerAddMappingsFromRW(rwops.get(), /* no close */ 0) < 0) {
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

glm::uvec2 SdlIoLayer::dimensions() const {
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(impl_->window.get(), &w, &h);
  return {static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)};
}

void SdlIoLayer::swap_buffers() {
  SDL_GL_SwapWindow(impl_->window.get());
}

void SdlIoLayer::capture_mouse(bool capture) {
  SDL_SetWindowGrab(impl_->window.get(), capture ? SDL_TRUE : SDL_FALSE);
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

    case SDL_AUDIODEVICEADDED:
    case SDL_AUDIODEVICEREMOVED:
      return event_type::kAudioDeviceChange;

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
        e.mods = impl_->keyboard_frame.mods();
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

std::size_t SdlIoLayer::controllers() const {
  return impl_->controllers.size();
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

void SdlIoLayer::controller_rumble(std::size_t index, std::uint16_t lf, std::uint16_t hf,
                                   std::uint32_t duration_ms) const {
  if (index < impl_->controllers.size()) {
    auto& data = impl_->controllers[index];
    SDL_JoystickRumble(SDL_GameControllerGetJoystick(data.controller.get()), lf, hf, duration_ms);
  }
}

void SdlIoLayer::set_audio_callback(const std::function<audio_callback>& callback) {
  std::unique_lock lock{audio_callback_lock()};
  impl_->audio_callback = callback;
}

void SdlIoLayer::close_audio_device() {
  impl_->audio_device.reset();
}

result<void> SdlIoLayer::open_audio_device(std::optional<std::size_t> index) {
  std::optional<std::string> device_name;
  if (index) {
    auto devices = audio_device_info();
    if (*index < devices.size()) {
      device_name = devices[*index];
    }
  }
  return impl_->open_audio_device(device_name);
}

std::vector<std::string> SdlIoLayer::audio_device_info() const {
  std::vector<std::string> result;
  for (int i = 0; i < SDL_GetNumAudioDevices(/* capture */ 0); ++i) {
    if (const char* name = SDL_GetAudioDeviceName(i, /* capture */ 0); name) {
      result.emplace_back(name);
    }
  }
  return result;
}

void SdlIoLayer::lock_audio_callback() {
  if (impl_->audio_device_id) {
    SDL_LockAudioDevice(impl_->audio_device_id);
  }
}

void SdlIoLayer::unlock_audio_callback() {
  if (impl_->audio_device_id) {
    SDL_UnlockAudioDevice(impl_->audio_device_id);
  }
}

}  // namespace ii::io