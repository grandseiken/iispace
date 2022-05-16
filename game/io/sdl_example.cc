#include "game/io/sdl_io.h"
#include <GL/gl3w.h>
#include <iostream>

int main() {
  auto result = ii::io::SdlIoLayer::create("example", /* GL major */ 4, /* GL minor */ 6);
  if (!result) {
    std::cerr << "ERROR: " << result.error() << std::endl;
    return 1;
  }
  std::cout << "Init OK" << std::endl;
  auto io_layer = std::move(*result);

  bool running = true;
  while (running) {
    io_layer->input_frame_clear();
    while (true) {
      auto event = io_layer->poll();
      if (!event) {
        break;
      }
      if (*event == ii::io::event_type::kClose) {
        running = false;
      }
    }
    glClear(GL_COLOR_BUFFER_BIT);
    io_layer->swap_buffers();

    auto frame = io_layer->controller_frame(0);
    std::cout << +frame.button_state[static_cast<std::size_t>(ii::io::controller::button::kX)]
              << " " << +frame.axis_state[static_cast<std::size_t>(ii::io::controller::axis::kLT)]
              << std::endl;
  }
  return 0;
}