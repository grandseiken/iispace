#include "game.h"
#include "lib.h"

Game::Game( Lib& lib )
: _lib( lib )
{
}

Game::~Game()
{
}
#include <iostream>
// Main loop
//------------------------------
Lib::ExitType Game::Run()
{
	while ( true ) {
        int f = _lib.GetFrameCount();
        if ( !f ) {
            if ( _lib.GetExitType() )
                return _lib.GetExitType();
            continue;
        }
        for ( int i = 0; i < f; ++i ) {
            _lib.BeginFrame();
            if ( _lib.GetExitType() ) {
                _lib.EndFrame();
                return _lib.GetExitType();
            }
            Update();
            if ( _lib.GetExitType() ) {
                _lib.EndFrame();
                return _lib.GetExitType();
            }
            _lib.EndFrame();
            if ( _lib.GetExitType() )
                return _lib.GetExitType();
        }
        _lib.ClearScreen();
        Render();
        _lib.Render();
	}
}
