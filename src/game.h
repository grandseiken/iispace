#ifndef IISPACE_SRC_GAME_H
#define IISPACE_SRC_GAME_H

#include "z0.h"
#include "lib.h"

class Game {
public:

  Game(Lib& lib);
  virtual ~Game();
  Lib::ExitType Run();

  virtual void Update() = 0;
  virtual void Render() const = 0;

  Lib& GetLib() const
  {
    return _lib;
  }

private:

  Lib& _lib;

};

#endif
