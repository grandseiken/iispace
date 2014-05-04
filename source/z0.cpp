#include "libwii.h"
#include "z0game.h"

int main( int argc, char** argv ) {

    LibWii* wii  = 0;
    z0Game* game = 0;

    try {
        wii = new LibWii();
        wii->Init();

        game = new z0Game( *wii );
        Lib::ExitType t = game->Run();

        delete game;
        game = 0;

        delete wii;
        wii = 0;

        // Debug
        // Gets to here OK

        if ( t == Lib::EXIT_TO_SYSTEM )
            SYS_ResetSystem( SYS_RETURNTOMENU, 0, 0 );
        else if ( t == Lib::EXIT_POWER_OFF )
            SYS_ResetSystem( SYS_POWEROFF, 0, 0 );
        else exit( 0 );

    }
    catch ( std::exception& e ) {
        if ( game ) delete game;
        if ( wii )  delete wii;
    }

    exit( 0 );
	return 0;
}
