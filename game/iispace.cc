#include "game/core/lib.h"
#include "game/core/ui_layer.h"
#include "game/core/z0_game.h"
#include "game/io/file/std_filesystem.h"
#include "game/io/sdl_io.h"
#include "game/render/gl_renderer.h"
#include <iostream>
#include <string>
#include <vector>

namespace ii {

bool run(const std::vector<std::string>& args) {
  // TODO: remove use of exceptions.
  try {
    static constexpr char kGlMajor = 4;
    static constexpr char kGlMinor = 6;
    auto io_layer_result = io::SdlIoLayer::create("iispace", kGlMajor, kGlMinor);
    if (!io_layer_result) {
      std::cerr << "Error initialising IO: " << io_layer_result.error() << std::endl;
      return false;
    }
    io::StdFilesystem fs{"assets", "savedata", "savedata/replays"};
    auto renderer_result = render::GlRenderer::create(fs);
    if (!renderer_result) {
      std::cerr << "Error initialising renderer: " << renderer_result.error() << std::endl;
      return false;
    }
    auto io_layer = std::move(*io_layer_result);
    auto renderer = std::move(*renderer_result);

    ui::UiLayer ui_layer{fs, *io_layer};
    Lib lib{fs, *io_layer, *renderer};
    z0Game game{fs, *io_layer, lib, args};

    bool exit = false;
    while (!exit) {
      if (lib.begin_frame()) {
        exit = true;
      }
      ui_layer.compute_input_frame();
      if (game.update(ui_layer)) {
        exit = true;
      }
      lib.end_frame();

      renderer->clear_screen();
      game.render(ui_layer);

      // TODO: too fast. Was 50FPS, now 60FPS, need timing logic.
      io_layer->swap_buffers();
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}

}  // namespace ii

int main(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return ii::run(args) ? 0 : 1;
}
