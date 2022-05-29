#include "game/core/lib.h"
#include "game/core/z0_game.h"
#include "game/io/file/std_filesystem.h"
#include "game/io/sdl_io.h"
#include "game/render/gl_renderer.h"
#include <iostream>
#include <string>
#include <vector>

bool run(const std::vector<std::string>& args) {
  // TODO: remove use of exceptions.
  try {
    static constexpr char kGlMajor = 4;
    static constexpr char kGlMinor = 6;
    auto io_layer = ii::io::SdlIoLayer::create("iispace", kGlMajor, kGlMinor);
    if (!io_layer) {
      std::cerr << "Error initialising IO: " << io_layer.error() << std::endl;
      return false;
    }
    ii::io::StdFilesystem fs{"assets", "savedata", "savedata/replays"};
    auto renderer = ii::render::GlRenderer::create(fs);
    if (!renderer) {
      std::cerr << "Error initialising renderer: " << renderer.error() << std::endl;
      return false;
    }
    Lib lib{fs, **io_layer, **renderer};
    z0Game game{lib, args};
    game.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return run(args) ? 0 : 1;
}
