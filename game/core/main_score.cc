#include "game/core/lib.h"
#include "game/core/z0_game.h"
#include "game/io/null_io.h"
#include "game/render/null_renderer.h"
#include <iostream>
#include <string>
#include <vector>

bool run(const std::vector<std::string>& args) {
  // TODO: remove use of exceptions.
  try {
    auto io_layer = ii::io::NullIoLayer::create();
    if (!io_layer) {
      std::cerr << "Error initialising IO: " << io_layer.error() << std::endl;
      return false;
    }
    auto renderer = ii::NullRenderer::create();
    if (!renderer) {
      std::cerr << "Error initialising renderer: " << renderer.error() << std::endl;
      return false;
    }
    Lib lib{/* headless */ true, **io_layer, **renderer};
    z0Game game{lib, args};
    game.run();
  } catch (const score_finished&) {
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}

bool test(const std::string& replay) {
  std::vector<std::string> args;
  args.emplace_back(replay);
  std::cout << "testing " << replay << std::endl;
  return run(args);
}

int main(int argc, char** argv) {
  std::vector<std::string> args;
  bool is_test = false;

  char* s = getenv("QUERY_STRING");
  if (s) {
    args.emplace_back(s);
  } else {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--test" || arg == "-test") {
        is_test = true;
        continue;
      }
      args.emplace_back(arg);
    }
  }
  std::cout << "Content-type: text/plain" << std::endl << std::endl;

  if (is_test) {
    test("tests/Darb_2p__Graves  Darb_553403.wrp");
    test("tests/Darb_4p__Team Graves_430987.wrp");
    test("tests/seiken_1p__crikey_641530.wrp");
    test("tests/seiken_2p__RAB  STU Yo_477833.wrp");
    test("tests/seiken_3p__3 OF US_219110.wrp");
  } else {
    return run(args) ? 0 : 1;
  }
}
