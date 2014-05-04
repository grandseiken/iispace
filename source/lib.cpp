#include "lib.h"

bool Lib::IsKeyPressed( Key k )
{
    for ( int i = 0; i < PLAYERS; i++ )
        if ( IsKeyPressed( i, k ) )
            return true;
    return false;
}

bool Lib::IsKeyReleased( Key k )
{
    for ( int i = 0; i < PLAYERS; i++ )
        if ( IsKeyReleased( i, k ) )
            return true;
    return false;
}

bool Lib::IsKeyHeld( Key k )
{
    for ( int i = 0; i < PLAYERS; i++ )
        if ( IsKeyHeld( i, k ) )
            return true;
    return false;
}
