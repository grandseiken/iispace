#include "game/core/lib.h"
#include "game/core/z0_game.h"
#include <iostream>
#include <string>
#include <vector>

std::int32_t run(const std::vector<std::string>& args) {
  try {
    Lib lib;
    z0Game game{lib, args};
    game.run();
  } catch (const score_finished&) {
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  return 0;
}

std::int32_t test(const std::string& replay) {
  std::vector<std::string> args;
  args.emplace_back(replay);
  std::cout << "testing " << replay << std::endl;
  return run(args);
}

std::int32_t main(std::int32_t argc, char** argv) {
  std::vector<std::string> args;
  bool is_test = false;

#ifdef PLATFORM_SCORE
  char* s = getenv("QUERY_STRING");
  if (s) {
    args.emplace_back(s);
  } else
    for (std::int32_t i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--test" || arg == "-test") {
        is_test = true;
        continue;
      }
      args.emplace_back(arg);
    }
  std::cout << "Content-type: text/plain" << std::endl << std::endl;
#else
  for (std::int32_t i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
#endif

  if (is_test) {
    test("tests/Darb_2p__Graves  Darb_553403.wrp");
    test("tests/Darb_4p__Team Graves_430987.wrp");
    test("tests/seiken_1p__crikey_641530.wrp");
    test("tests/seiken_2p__RAB  STU Yo_477833.wrp");
    test("tests/seiken_3p__3 OF US_219110.wrp");
  } else {
    return run(args);
  }
}
