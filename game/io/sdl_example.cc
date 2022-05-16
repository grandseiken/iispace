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

  while (true) {
    io_layer->update();
    if (io_layer->should_close()) {
      break;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    io_layer->swap_buffers();
  }
  return 0;
}