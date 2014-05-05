#ifndef z0_H
#define z0_H

#include <string>
#include <vector>
#include <sstream>
#include <math.h>
#include <cmath>
#include <cstdlib>

#define USE_FIX32

#ifdef USE_MPREAL
#define MPFR_PRECISION 32
#include "../mpfrc++/mpreal.h"
#endif
#ifdef USE_FIX32
#include "fix32.h"
#endif

// Forward declarations
//------------------------------
typedef unsigned int Colour;
#ifdef USE_FLOAT
typedef float fixed;
#endif
#ifdef USE_FIX32
typedef Fix32 fixed;
#endif
#ifdef USE_MPREAL
typedef mpfr::mpreal fixed;
#endif
class Lib;
class Game;
class z0Game;
class Shape;
class Ship;
class Enemy;
class Player;
class Overmind;
class DeathRayBoss;
class DeathRayArm;

// Deterministic math
//------------------------------
#define Z_RAND_MAX 0x7fff
void z_srand( int seed );
int z_rand();

int   z_int( fixed x );
float z_float( fixed x );
fixed z_abs( fixed x );
fixed z_sqrt( fixed x );
fixed z_sin( fixed x );
fixed z_cos( fixed x );
fixed z_atan2( fixed y, fixed x );

// Vector math
//------------------------------
#undef M_PI
#undef M_2PI
#undef M_3PI
#ifdef USE_FLOAT

#define M_PI 3.14159265358979323846264338327f
#define M_PIf 3.14159265358979323846264338327f
#define M_2PI ( 2.0f * M_PI )
#define M_3PI ( 3.0f * M_PI )
#define M_ZERO ( 0.0f )
#define M_HALF ( 0.5f )
#define M_ONE   ( 1.0f )
#define M_TWO   ( 2.0f )
#define M_THREE ( 3.0f )
#define M_FOUR  ( 4.0f )
#define M_FIVE  ( 5.0f )
#define M_SIX   ( 6.0f )
#define M_SEVEN ( 7.0f )
#define M_EIGHT ( 8.0f )
#define M_TEN   ( 10.0f )
#define M_PT_ZERO_ONE ( 0.01f )
#define M_PT_ONE   ( 0.1f )

#endif
#ifdef USE_FIX32

const fixed M_PI          = F32PI;
const fixed M_2PI         = F32PI * 2;
const fixed M_3PI         = F32PI * 3;
const fixed M_ZERO        = 0;
const fixed M_ONE         = 1;
const fixed M_HALF        = fixed( 1 ) / fixed( 2 );
const fixed M_TWO         = 2;
const fixed M_THREE       = 3;
const fixed M_FOUR        = 4;
const fixed M_FIVE        = 5;
const fixed M_SIX         = 6;
const fixed M_SEVEN       = 7;
const fixed M_EIGHT       = 8;
const fixed M_TEN         = 10;
const fixed M_PT_ZERO_ONE = fixed( 1 ) / fixed( 100 );
const fixed M_PT_ONE      = fixed( 1 ) / fixed( 10 );
#define M_PIf 3.14159265358979323846264338327f

#endif
#ifdef USE_MPREAL

const fixed M_PI          = 3.14159265358979323846264338327;
const fixed M_2PI         = fixed( 2 ) * M_PI;
const fixed M_3PI         = fixed( 3 ) * M_PI;
const fixed M_ZERO        = fixed( 0 );
const fixed M_ONE         = fixed( 1 );
const fixed M_HALF        = fixed( 1 ) / fixed( 2 );
const fixed M_TWO         = fixed( 2 );
const fixed M_THREE       = fixed( 3 );
const fixed M_FOUR        = fixed( 4 );
const fixed M_FIVE        = fixed( 5 );
const fixed M_SIX         = fixed( 6 );
const fixed M_SEVEN       = fixed( 7 );
const fixed M_EIGHT       = fixed( 8 );
const fixed M_TEN         = fixed( 10 );
const fixed M_PT_ZERO_ONE = fixed( 1 ) / fixed( 100 );
const fixed M_PT_ONE      = fixed( 1 ) / fixed( 10 );
#define M_PIf 3.14159265358979323846264338327f

#endif

class Vec2 {
public:

    fixed _x;
    fixed _y;

    Vec2()
    : _x( 0 ), _y( 0 ) { }
    Vec2( fixed x, fixed y )
    : _x( x ), _y( y ) { }
    Vec2( const Vec2& a )
    : _x( a._x ), _y( a._y ) { }

    fixed Length() const
    {
        return z_sqrt( _x * _x + _y * _y );
    }
    fixed Angle() const
    {
        return z_atan2( _y, _x );
    }

    void Set( fixed x, fixed y )
    {
        _x = x;
        _y = y;
    }
    void SetPolar( fixed angle, fixed length )
    {
        _x = length * z_cos( angle );
        _y = length * z_sin( angle );
    }
    void Normalise()
    {
        fixed l = Length();
        if ( l <= M_ZERO )
            return;
        _x /= l;
        _y /= l;
    }
    void Rotate( fixed angle )
    {
        if ( angle != 0 )
            SetPolar( Angle() + angle, Length() );
    }
    fixed DotProduct( const Vec2& a ) const
    {
        return _x * a._x + _y * a._y;
    }

    Vec2 operator+( const Vec2& a ) const
    {
        return Vec2( _x + a._x, _y + a._y );
    }

    Vec2 operator-( const Vec2& a ) const
    {
        return Vec2( _x - a._x, _y - a._y );
    }

    Vec2 operator+=( const Vec2& a )
    {
        _x += a._x;
        _y += a._y;
        return *this;
    }

    Vec2 operator-=( const Vec2& a )
    {
        _x -= a._x;
        _y -= a._y;
        return *this;
    }

    Vec2 operator*( fixed a ) const
    {
        return Vec2( _x * a, _y * a );
    }

    Vec2 operator/( fixed a ) const
    {
        return Vec2( _x / a, _y / a );
    }

    Vec2 operator*=( fixed a )
    {
        _x *= a;
        _y *= a;
        return *this;
    }

    Vec2 operator/=( fixed a )
    {
        _x /= a;
        _y /= a;
        return *this;
    }

    bool operator==( const Vec2& a ) const
    {
        return _x == a._x && _y == a._y;
    }

    bool operator!=( const Vec2& a ) const
    {
        return _x != a._x || _y != a._y;
    }

    Vec2& operator=( const Vec2& a )
    {
        _x = a._x;
        _y = a._y;
        return *this;
    }

};

#ifdef USE_FLOAT
typedef Vec2 Vec2f;
#else
class Vec2f {
public:

    float _x;
    float _y;

    Vec2f()
    : _x( 0 ), _y( 0 ) { }
    explicit Vec2f( float x, float y )
    : _x( x ), _y( y ) { }
    explicit Vec2f( const Vec2& a )
    : _x( z_float( a._x ) ), _y( z_float( a._y ) ) { }

    float Length() const
    {
        return sqrt( _x * _x + _y * _y );
    }
    float Angle() const
    {
        return atan2( _y, _x );
    }

    void Set( float x, float y )
    {
        _x = x;
        _y = y;
    }
    void SetPolar( float angle, float length )
    {
        _x = length * cos( angle );
        _y = length * sin( angle );
    }
    void Normalise()
    {
        float l = Length();
        if ( l <= 0 )
            return;
        _x /= l;
        _y /= l;
    }
    void Rotate( float angle )
    {
        SetPolar( Angle() + angle, Length() );
    }
    float DotProduct( const Vec2f& a ) const
    {
        return _x * a._x + _y * a._y;
    }

    Vec2f operator+( const Vec2f& a ) const
    {
        return Vec2f( _x + a._x, _y + a._y );
    }

    Vec2f operator-( const Vec2f& a ) const
    {
        return Vec2f( _x - a._x, _y - a._y );
    }

    Vec2f operator+=( const Vec2f& a )
    {
        _x += a._x;
        _y += a._y;
        return *this;
    }

    Vec2f operator-=( const Vec2f& a )
    {
        _x -= a._x;
        _y -= a._y;
        return *this;
    }

    Vec2f operator*( float a ) const
    {
        return Vec2f( _x * a, _y * a );
    }

    Vec2f operator/( float a ) const
    {
        return Vec2f( _x / a, _y / a );
    }

    Vec2f operator*=( float a )
    {
        _x *= a;
        _y *= a;
        return *this;
    }

    Vec2f operator/=( float a )
    {
        _x /= a;
        _y /= a;
        return *this;
    }

    bool operator==( const Vec2f& a ) const
    {
        return _x == a._x && _y == a._y;
    }

    bool operator!=( const Vec2f& a ) const
    {
        return _x != a._x || _y != a._y;
    }

    Vec2f& operator=( const Vec2f& a )
    {
        _x = a._x;
        _y = a._y;
        return *this;
    }

};
#endif

#endif
