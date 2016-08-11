#include "lib.h"
#include "z0_game.h"

#include <iostream>
#include <string>
#include <vector>

int32_t run(const std::vector<std::string>& args) {
  try {
    Lib lib;
    z0Game game(lib, args);
    game.run();
  } catch (const score_finished&) {
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int32_t test(const std::string& replay) {
  std::vector<std::string> args;
  args.push_back(replay);
  std::cout << "testing " << replay << std::endl;
  return run(args);
}

int32_t main(int32_t argc, char** argv) {
  std::vector<std::string> args;
  bool is_test = false;

#ifdef PLATFORM_SCORE
  char* s = getenv("QUERY_STRING");
  if (s) {
    args.push_back(std::string(s));
  } else
    for (int32_t i = 1; i < argc; ++i) {
      if (argv[i] == std::string("--test") || argv[i] == std::string("-test")) {
        is_test = true;
        continue;
      }
      args.push_back(std::string(argv[i]));
    }
  std::cout << "Content-type: text/plain" << std::endl << std::endl;
#else
  for (int32_t i = 1; i < argc; ++i) {
    args.push_back(std::string(argv[i]));
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
