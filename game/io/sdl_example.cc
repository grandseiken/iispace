#include "game/io/null_io.h"
#include "game/io/sdl_io.h"
#include "game/render/gl_renderer.h"
#include "game/render/null_renderer.h"
#include <cmath>
#include <iostream>
#include <limits>

int main() {
  auto io_result = ii::io::SdlIoLayer::create("example", /* GL major */ 4, /* GL minor */ 6);
  if (!io_result) {
    std::cerr << "ERROR (IO): " << io_result.error() << std::endl;
    return 1;
  }
  std::cout << "Init OK" << std::endl;
  auto io_layer = std::move(*io_result);

  auto renderer_result = ii::GlRenderer::create();
  if (!renderer_result) {
    std::cerr << "ERROR (renderer): " << renderer_result.error() << std::endl;
    return 1;
  }
  auto renderer = std::move(*renderer_result);

  std::uint64_t sin_wave = 0;
  auto audio_callback = [&sin_wave](std::uint8_t* out_buffer, std::size_t samples) {
    for (std::size_t i = 0; i < samples; ++i) {
      float f = static_cast<float>(sin_wave) * 0.05f;
      auto x = static_cast<ii::io::audio_sample_t>(
          std::sin(f) * .5f * std::numeric_limits<ii::io::audio_sample_t>::max());
      std::memcpy(out_buffer, &x, sizeof(x));
      std::memcpy(out_buffer + sizeof(x), &x, sizeof(x));
      out_buffer += 2 * sizeof(x);
      sin_wave++;
    }
  };

  io_layer->set_audio_callback(audio_callback);
  bool running = true;
  while (running) {
    io_layer->input_frame_clear();
    bool audio_change = false;
    bool controller_change = false;
    while (true) {
      auto event = io_layer->poll();
      if (!event) {
        break;
      }
      if (*event == ii::io::event_type::kClose) {
        running = false;
      } else if (*event == ii::io::event_type::kAudioDeviceChange) {
        audio_change = true;
      } else if (*event == ii::io::event_type::kControllerChange) {
        controller_change = true;
      }
    }

    if (controller_change) {
      auto infos = io_layer->controller_info();
      std::size_t i = 0;
      for (const auto& info : infos) {
        std::cout << "Controller " << i++ << ": " << info.name << std::endl;
      }
    }

    if (audio_change) {
      auto infos = io_layer->audio_device_info();
      std::size_t i = 0;
      for (const auto& info : infos) {
        std::cout << "Audio device " << i++ << ": " << info << std::endl;
      }

      auto result = io_layer->open_audio_device(std::nullopt);
      if (result) {
        std::cout << "Audio device OK" << std::endl;
      } else {
        std::cerr << "ERROR: " << result.error() << std::endl;
      }
    }

    renderer->set_dimensions(io_layer->dimensions(), glm::uvec2{640, 480});
    renderer->clear_screen();
    renderer->render_legacy_rect({0, 0}, {640, 480}, 4, {1.f, 1.f, 1.f, 1.f});
    renderer->render_legacy_line({0, 0}, {640, 480}, {1.f, 1.f, 1.f, 1.f});
    renderer->render_legacy_text({1, 1}, {1.f, 1.f, 1.f, 1.f}, "HELLO!");
    renderer->render_legacy_text({1, 2}, {1.f, 0.f, 0.f, 1.f}, "THIS TEXT IS RED!");
    renderer->render_legacy_text({1, 3}, {1.f, 0.f, 1.f, 1.f}, "HELLO HELLO HELLO HELLO!");
    io_layer->swap_buffers();

    auto frame = io_layer->controller_frame(0);
    std::cout << +frame.button(ii::io::controller::button::kX) << " "
              << +frame.axis(ii::io::controller::axis::kLT) << std::endl;
  }
  io_layer->close_audio_device();
  return 0;
}