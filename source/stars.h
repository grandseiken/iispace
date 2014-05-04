#ifndef STARS_H
#define STARS_H

#include "z0game.h"

class Star {
public:

    static const int TIMER = 500;
    Star( z0Game& z0, float speed );
    virtual ~Star();

    static void Update();
    static void Render();
    static void CreateStar( z0Game& z0 );
    static void SetDirection( const Vec2& direction );
    static void Clear();

private:

    bool Move();
    void ResetTimer();
    virtual void RenderStar( Lib& lib, const Vec2& position ) = 0;

    int     _timer;
    Vec2    _position;
    float   _speed;
    z0Game& _z0;

    static Vec2 _direction;
    typedef std::vector< Star* > StarList;
    static StarList _starList;

};

class DotStar : public Star {
public:

    DotStar( z0Game& z0 ) : Star( z0, 18 ) { _c = z0.GetLib().RandInt( 2 ) ? 0x222222ff : 0x333333ff; }
    virtual ~DotStar() { }

private:

    virtual void RenderStar( Lib& lib, const Vec2& position )
    { lib.RenderRect( position - Vec2( 1, 1 ), position + Vec2( 1, 1 ), _c ); }

    Colour _c;

};

class FarStar : public Star {
public:

    FarStar( z0Game& z0 ) : Star( z0, 10 ) { _c = z0.GetLib().RandInt( 2 ) ? 0x222222ff : 0x111111ff; }
    virtual ~FarStar() { }

private:

    virtual void RenderStar( Lib& lib, const Vec2& position )
    { lib.RenderRect( position - Vec2( 1, 1 ), position + Vec2( 1, 1 ), _c ); }

    Colour _c;

};

class BigStar : public Star {
public:

    BigStar( z0Game& z0 ) : Star( z0, 14 ) { _c = z0.GetLib().RandInt( 2 ) ? 0x111111ff : 0x222222ff; }
    virtual ~BigStar() { }

private:

    virtual void RenderStar( Lib& lib, const Vec2& position )
    { lib.RenderRect( position - Vec2( 2, 2 ), position + Vec2( 2, 2 ), _c ); }

    Colour _c;

};

class PlanetStar : public Star {
public:

    PlanetStar( z0Game& z0 )
    : Star( z0, 10 )
    { _n = z0.GetLib().RandInt( 4 ) + 4; }
    virtual ~PlanetStar() { }

private:

    virtual void RenderStar( Lib& lib, const Vec2& position )
    {
        for ( int i = 0; i < 8; i++ ) {
            Vec2 a, b;
            a.SetPolar( i * M_PI / 4.0f, _n );
            b.SetPolar( ( i + 1 ) * M_PI / 4.0f, _n );
            lib.RenderLine( position + a, position + b, 0x111111ff );
        }
    }

    Colour _c;
    int _n;

};

#endif
