#ifndef GAME_H
#define GAME_H

#include "z0.h"
#include "lib.h"

class Game {
public:

    // General
    //------------------------------
    Game( Lib& lib );
    virtual ~Game();
    Lib::ExitType Run();

    // Game functions
    //------------------------------
    virtual void Update() = 0;
    virtual void Render() = 0;

    // Library accessor
    //------------------------------
    Lib& GetLib() const
    { return _lib; }

private:

    Lib&      _lib;

};

#endif
