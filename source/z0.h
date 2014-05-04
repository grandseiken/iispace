#ifndef z0_H
#define z0_H

#include <string>
#include <vector>
#include <sstream>
#include <math.h>

// Forward declarations
//------------------------------
typedef unsigned int Colour;
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

// Vector math
//------------------------------
class Vec2 {
public:

    float _x;
    float _y;

    Vec2()
    : _x( 0 ), _y( 0 ) { }
    Vec2( float x, float y )
    : _x( x ), _y( y ) { }
    Vec2( const Vec2& a )
    : _x( a._x ), _y( a._y ) { }

    float Length() const
    { return sqrt( _x * _x + _y * _y ); }
    float Angle() const
    {
        if ( _x == 0.f ) return _y > 0.f ? M_PI / 2.f : 3.f * M_PI / 2.f;
        float oa = _y / _x;
        if ( oa < 0.f ) oa = -oa;
        float a = atan( oa );
        if ( _x < 0.f && _y >= 0.f ) return M_PI - a;
        if ( _x < 0.f && _y <= 0.f ) return M_PI + a;
        if ( _x > 0.f && _y <= 0.f ) return 2.f * M_PI - a;
        return a;
    }

    void Set( float x, float y )
    { _x = x; _y = y; }
    void SetPolar( float angle, float length )
    { _x = length * cos( angle ); _y = length * sin( angle ); }
    void Normalise()
    { float l = Length(); if ( l <= 0 ) return; _x /= l; _y /= l; }
    void Rotate( float angle )
    { SetPolar( Angle() + angle, Length() ); }
    float DotProduct( const Vec2& a ) const
    { return _x * a._x + _y * a._y; }

    Vec2 operator+( const Vec2& a ) const
    { return Vec2( _x + a._x, _y + a._y ); }

    Vec2 operator-( const Vec2& a ) const
    { return Vec2( _x - a._x, _y - a._y ); }

    Vec2 operator+=( const Vec2& a )
    { _x += a._x; _y += a._y; return *this; }

    Vec2 operator-=( const Vec2& a )
    { _x -= a._x; _y -= a._y; return *this; }

    Vec2 operator*( float a ) const
    { return Vec2( _x * a, _y * a ); }

    Vec2 operator/( float a ) const
    { return Vec2( _x / a, _y / a ); }

    Vec2 operator*=( float a )
    { _x *= a; _y *= a; return *this; }

    Vec2 operator/=( float a )
    { _x /= a; _y /= a; return *this; }

    bool operator==( const Vec2& a ) const
    { return _x == a._x && _y == a._y; }

    bool operator!=( const Vec2& a ) const
    { return _x != a._x || _x != a._x; }

    Vec2 operator=( const Vec2& a )
    { _x = a._x; _y = a._y; return *this; }

};

#endif
