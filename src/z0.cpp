#include "z0_game.h"
#include <iostream>
#ifdef PLATFORM_IISPACE
#include "lib_ii.h"
#endif
#ifdef PLATFORM_SCORE
#include "lib_score.h"
#include <cstdlib>
#include <vector>
#endif

int run(const std::vector<std::string>& args)
{
  Lib* lib = 0;
  z0Game* game = 0;
  try {
#ifdef PLATFORM_IISPACE
    lib = new LibWin();
#endif
#ifdef PLATFORM_SCORE
    lib = new LibRepScore();
#endif
    lib->Init();

    game = new z0Game(*lib, args);
    Lib::ExitType t = game->Run();

    delete game;
    game = 0;

    if (t == Lib::EXIT_TO_SYSTEM) {
      lib->SystemExit(false);
    }
    else if (t == Lib::EXIT_POWER_OFF) {
      lib->SystemExit(true);
    }
    else {
      exit(0);
    }

    delete lib;
    lib = 0;
    return 0;
  }
  catch (const score_finished&) {
    return 0;
    if (game) {
      delete game;
    }
    if (lib) {
      delete lib;
    }
  }
  catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    if (game) {
      delete game;
    }
    if (lib) {
      delete lib;
    }
    return 1;
  }
}

int test(const std::string& replay)
{
  std::vector<std::string> args;
  args.push_back(replay);
  std::cout << "testing " << replay << std::endl;
  return run(args);
}

int main(int argc, char** argv)
{
  std::vector<std::string> args;
  bool is_test = false;

#ifdef PLATFORM_SCORE
  char* s = getenv("QUERY_STRING");
  if (s) {
    args.push_back(std::string(s));
  }
  else for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("--test") ||
        argv[i] == std::string("-test")) {
      is_test = true;
      continue;
    }
    args.push_back(std::string(argv[i]));
  }
  std::cout << "Content-type: text/plain" << std::endl << std::endl;
#else
  for (int i = 1; i < argc; ++i) {
    args.push_back(std::string(argv[i]));
  }
#endif

  if (is_test) {
    test("tests/Darb_2p__Graves  Darb_553403.wrp");
    test("tests/Darb_4p__Team Graves_430987.wrp");
    test("tests/seiken_1p__crikey_641530.wrp");
    test("tests/seiken_2p__RAB  STU Yo_477833.wrp");
    test("tests/seiken_3p__3 OF US_219110.wrp");
  }
  else {
    return run(args);
  }
}
