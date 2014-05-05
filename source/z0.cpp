#include "z0game.h"
#include <iostream>
#ifdef PLATFORM_WII
#include "libwii.h"
#endif
#ifdef PLATFORM_WIN
#include "libwin.h"
#endif
#ifdef PLATFORM_REPSCORE
#include "librepscore.h"
#include <cstdlib>
#include <vector>
int main( int argc, char** argv )
{
    std::vector< std::string > args;
    char* s = getenv( "QUERY_STRING" );
    if ( s )
        args.push_back( std::string( s ) );
    else for ( int i = 1; i < argc; ++i )
        args.push_back( std::string( argv[ i ] ) );
    std::cout << "Content-type: text/plain" << std::endl << std::endl;
#else
int main( int argc, char** argv )
{
    std::vector< std::string > args;
    for ( int i = 1; i < argc; ++i )
        args.push_back( std::string( argv[ i ] ) );
#endif

    Lib*    lib  = 0;
    z0Game* game = 0;

#ifdef USE_MPREAL
    mpfr::mpreal::set_default_prec( MPFR_PRECISION );
#endif

    try {
#ifdef PLATFORM_WII
        lib = new LibWii();
#endif
#ifdef PLATFORM_WIN
        lib = new LibWin();
#endif
#ifdef PLATFORM_REPSCORE
        lib = new LibRepScore();
#endif
        lib->Init();

        game = new z0Game( *lib, args );
        Lib::ExitType t = game->Run();

        delete game;
        game = 0;

        if ( t == Lib::EXIT_TO_SYSTEM )
            lib->SystemExit( false );
        else if ( t == Lib::EXIT_POWER_OFF )
            lib->SystemExit( true );
        else
            exit( 0 );

        delete lib;
        lib = 0;

    }
    catch ( std::exception& e ) {
        std::cout << e.what() << std::endl;
        if ( game )
            delete game;
        if ( lib )
            delete lib;
    }

	return 0;
}
