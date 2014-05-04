#include "game.h"
#include "lib.h"

Game::Game( Lib& lib )
: _lib( lib )
{
}

Game::~Game()
{
}

// Main loop
//------------------------------
Lib::ExitType Game::Run()
{
	while ( true ) {
        _lib.BeginFrame();
        Update();
        _lib.ClearScreen();
        Render();
		_lib.EndFrame();
        if ( _lib.GetExitType() )
            return _lib.GetExitType();
	}
}
