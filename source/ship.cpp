#include "ship.h"
#include "z0Game.h"

Ship::Ship( const Vec2& position )
: _z0( 0 )
, _destroy( false )
, _position( position )
, _rotation( 0 )
, _boundingWidth( 0 )
{
}

Ship::~Ship()
{
    for ( unsigned int i = 0; i < CountShapes(); i++ )
        delete _shapeList[ i ];
}

void Ship::AddShape( Shape* shape )
{
    _shapeList.push_back( shape );
}

void Ship::DestroyShape( int i )
{
    if ( i < 0 || i >= int( CountShapes() ) )
        return;
    delete _shapeList[ i ];
    _shapeList.erase( _shapeList.begin() + i );
}

bool Ship::CheckPoint( const Vec2& v, int category ) const
{
    Vec2 a = v - GetPosition();
    a.Rotate( -GetRotation() );
    for ( unsigned int i = 0; i < CountShapes(); i++ ) {
        if ( GetShape( i ).GetCategory() && ( !category || GetShape( i ).GetCategory() & category ) ) {
            if ( GetShape( i ).CheckPoint( a ) )
                return true;
        }
    }
    return false;
}

void Ship::Render()
{
    for ( unsigned int i = 0; i < CountShapes(); i++ ) {
        GetShape( i ).Render( GetLib(), GetPosition(), GetRotation() );
    }
}

void Ship::RenderWithColour( Colour colour )
{
    for ( unsigned int i = 0; i < CountShapes(); i++ ) {
        GetShape( i ).Render( GetLib(), GetPosition(), GetRotation(), colour );
    }
}

Shape& Ship::GetShape( int i ) const
{
    return *_shapeList[ i ];
}

unsigned int Ship::CountShapes() const
{
    return _shapeList.size();
}

Lib& Ship::GetLib() const
{
    return _z0->GetLib();
}

void Ship::Destroy()
{
    if ( IsDestroyed() ) return;
    _destroy = true;
    for ( unsigned int i = 0; i < CountShapes(); i++ )
        GetShape( i ).SetCategory( 0 );
}

void Ship::Spawn( Ship* ship ) const
{
    _z0->AddShip( ship );
}

void Ship::Explosion( Colour c, int time ) const
{
    for ( unsigned int i = 0; i < CountShapes(); i++ ) {
        int n = GetLib().RandInt( 8 ) + 8;
        for ( int j = 0; j < n; j++ ) {
            Vec2 pos = GetShape( i ).ConvertPoint( GetPosition(), GetRotation(), Vec2() );
            Vec2 dir;
            dir.SetPolar( GetLib().RandFloat() * 2.0f * M_PI, 6.0f );
            Spawn( new Particle( pos, c ? c : GetShape( i ).GetColour(), dir, time + GetLib().RandInt( 8 ) ) );
        }
    }
}
