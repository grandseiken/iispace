#include "stars.h"

Vec2           Star::_direction( 0, 1 );
Star::StarList Star::_starList;

Star::Star( z0Game& z0, float speed )
: _timer( TIMER )
, _speed( speed )
, _z0( z0 )
{
    _starList.push_back( this );
    int edge = _z0.GetLib().RandInt( 4 );
    float ratio = _z0.GetLib().RandFloat();
    if ( edge < 2 )
        _position._x = ratio * Lib::WIDTH;
    else
        _position._y = ratio * Lib::HEIGHT;
    
    if ( edge == 0 )
        _position._y = -16;
    if ( edge == 1 )
        _position._y = Lib::HEIGHT + 16;
    if ( edge == 2 )
        _position._x = -16;
    if ( edge == 3 )
        _position._x = Lib::WIDTH + 16;
}

Star::~Star()
{ }

void Star::Update()
{
    for ( int i = 0; i < int( _starList.size() ); i++ ) {
        if ( !_starList[ i ]->Move() ) {
            delete _starList[ i ];
            _starList.erase( _starList.begin() + i );
            i--;
        }
    }
}

void Star::Render()
{
    for ( unsigned int i = 0; i < _starList.size(); i++ ) {
        _starList[ i ]->RenderStar( _starList[ i ]->_z0.GetLib(), _starList[ i ]->_position );
    }
}

void Star::CreateStar( z0Game& z0 )
{
    int r = z0.GetLib().RandInt( 12 );
    if ( r <= 0 ) {
        if ( z0.GetLib().RandInt( 4 ) == 0 )
            new PlanetStar( z0 );
    }
    else if ( r <= 3 )
        new BigStar( z0 );
    else if ( r <= 7 )
        new FarStar( z0 );
    else
        new DotStar( z0 );
}

void Star::SetDirection( const Vec2& direction )
{
    _direction = direction;
    for ( unsigned int i = 0; i < _starList.size(); i++ ) {
        _starList[ i ]->ResetTimer();
    }
}

void Star::Clear()
{
    for ( unsigned int i = 0; i < _starList.size(); i++ ) {
        delete _starList[ i ];
    }
    _starList.clear();
}

bool Star::Move()
{
    _position += _direction * _speed;
    _timer--;
    return _timer > 0;
}

void Star::ResetTimer()
{
    _timer = TIMER;
}
