#include "z0.h"
#include <cstdint>

#ifdef USE_FLOAT
fixed z_sqrt( fixed number )
{
#ifdef USE_SQRT
    float x = number * 0.5f;
    float y = number;
    int32_t i = *( int32_t* )&y;
    i = 0x5f3759df - ( i >> 1 );
    y = *( float* )&i;
    y = y * ( 1.5f - ( x * y * y ) );
    y = y * ( 1.5f - ( x * y * y ) );
    return number * y;
#else
    return sqrt( number );
#endif
}
#endif
