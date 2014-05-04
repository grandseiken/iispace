#include "enemy.h"
#include "player.h"

// Basic enemy
//------------------------------
Enemy::Enemy( const Vec2& position, int hp )
: Ship( position )
, _hp( hp )
, _score( 0 )
, _damaged( false )
, _destroySound( Lib::SOUND_ENEMY_DESTROY )
{
}

void Enemy::Damage( int damage, Player* source ) {
    _hp -= damage;
    PlaySoundRandom( Lib::SOUND_ENEMY_HIT );

    if ( _hp <= 0 && !IsDestroyed() ) {
        PlaySoundRandom( _destroySound );
        if ( source )
            source->AddScore( GetScore() );
        Explosion();
        OnDestroy( damage >= Player::BOMB_DAMAGE );
        Destroy();
    } else if ( !IsDestroyed() ) {
        PlaySoundRandom( Lib::SOUND_ENEMY_HIT );
        _damaged = true;
    }
}

void Enemy::Render()
{
    if ( !_damaged ) {
        Ship::Render();
        return;
    }
    RenderWithColour( 0xffffffff );
    _damaged = false;
}

// Follower enemy
//------------------------------
Follow::Follow( const Vec2& position, float radius, int hp )
: Enemy( position, hp )
, _timer( 0 )
, _target( 0 )
{
    AddShape( new Polygon( Vec2(), radius, 4, 0x9933ffff, 0, ENEMY | VULNERABLE ) );
    SetScore( 15 );
    SetBoundingWidth( 10 );
    SetDestroySound( Lib::SOUND_ENEMY_SHATTER );
}

void Follow::Update()
{
    Rotate( 0.1f );
    if ( Player::CountKilledPlayers() >= int( GetPlayers().size() ) )
        return;

    _timer++;
    if ( !_target || _timer > TIME ) {
        _target = GetNearestPlayer();
        _timer = 0;
    }
    Vec2 d = _target->GetPosition() - GetPosition();
    if ( d.Length() > 0 ) {
        d.Normalise();
        d *= SPEED;

        Move( d );
    }
}

// Chaser enemy
//------------------------------
Chaser::Chaser( const Vec2& position )
: Enemy( position, 2 )
, _move( false )
, _timer( TIME )
, _dir()
{
    AddShape( new Polygram( Vec2(), 10, 4, 0x3399ffff, 0, ENEMY | VULNERABLE ) );
    SetScore( 30 );
    SetBoundingWidth( 10 );
    SetDestroySound( Lib::SOUND_ENEMY_SHATTER );
}

void Chaser::Update()
{
    bool before = IsOnScreen();

    if ( _timer )
        _timer--;
    if ( _timer <= 0 ) {
        _timer = TIME * ( _move + 1 );
        _move = !_move;
        if ( _move ) {
            Ship* p = GetNearestPlayer();
            _dir = p->GetPosition() - GetPosition();
            _dir.Normalise();
            _dir *= SPEED;
        }
    }
    if ( _move ) {
        Move( _dir );
        if ( !before && IsOnScreen() ) {
            _move = false;
        }
    }
    else {
        Rotate( 0.1f );
    }
}

// Square enemy
//------------------------------
Square::Square( const Vec2& position, float rotation )
: Enemy( position, 4 )
, _dir()
{
    AddShape( new Box( Vec2(), 10, 10, 0x33ff33ff, 0, ENEMY | VULNERABLE ) ); _dir.SetPolar( rotation, 1.f );
    SetScore( 25 );
    SetBoundingWidth( 15 );
}

void Square::Update()
{
    Vec2 pos = GetPosition();
    if ( pos._x < 0 && _dir._x <= 0.f ) {
        _dir._x = -_dir._x;
        if ( _dir._x <= 0.f ) _dir._x = 1;
    }
    if ( pos._y < 0 && _dir._y <= 0.f ) {
        _dir._y = -_dir._y;
        if ( _dir._y <= 0.f ) _dir._y = 1;
    }

    if ( pos._x > Lib::WIDTH && _dir._x >= 0.f ) {
        _dir._x = -_dir._x;
        if ( _dir._x >= 0.f ) _dir._x = -1;
    }
    if ( pos._y > Lib::HEIGHT && _dir._y >= 0.f ) {
        _dir._y = -_dir._y;
        if ( _dir._y >= 0.f ) _dir._y = -1;
    }
    _dir.Normalise();

    Move( _dir * SPEED );
    SetRotation( _dir.Angle() );
}

// Wall enemy
//------------------------------
Wall::Wall( const Vec2& position )
: Enemy( position, 10 )
, _dir( 0, 1 )
, _timer( 0 )
, _rotate( false )
{
    AddShape( new Box( Vec2(), 10, 40, 0x33cc33ff, 0, ENEMY | VULNERABLE ) );
    SetScore( 20 );
    SetBoundingWidth( 50 );
}

void Wall::Update()
{
    if ( _rotate ) {
        Vec2 d( _dir );
        d.Rotate( ( TIMER - _timer ) * M_PI / ( 4.0f * float( TIMER ) ) );
        SetRotation( d.Angle() );
        _timer--;
        if ( _timer <= 0 ) {
            _timer = 0;
            _rotate = false;
            _dir.Rotate( M_PI / 4.0f );
        }
        return;
    }
    else {
        _timer++;
        if ( _timer > TIMER * 6 ) {
            _timer = TIMER;
            _rotate = true;
        }
    }

    Vec2 pos = GetPosition();
    if ( ( pos._x < 0 && _dir._x < 0 ) ||
         ( pos._y < 0 && _dir._y < 0 ) ||
         ( pos._x > Lib::WIDTH && _dir._x > 0 ) ||
         ( pos._y > Lib::HEIGHT && _dir._y > 0 ) ) {
        _dir = Vec2() - _dir;
        _dir.Normalise();
    }

    Move( _dir * SPEED );
    SetRotation( _dir.Angle() );
}

void Wall::OnDestroy( bool bomb )
{
    if ( bomb )
        return;
    Vec2 d = _dir;
    d.Rotate( M_PI / 2.0f );
    Vec2 v = GetPosition() + d * 30.0f;
    if ( v._x >= 0 && v._x <= Lib::WIDTH && v._y >= 0 && v._y <= Lib::HEIGHT )
        Spawn( new Square( v, GetRotation() ) );
    v = GetPosition() - d * 30.0f;
    if ( v._x >= 0 && v._x <= Lib::WIDTH && v._y >= 0 && v._y <= Lib::HEIGHT )
        Spawn( new Square( v, GetRotation() ) );
}

// Follow spawner enemy
//------------------------------
FollowHub::FollowHub( const Vec2& position )
: Enemy( position, 14 )
, _timer( 0 )
, _dir( 0, 0 )
, _count( 0 )
{
    AddShape( new Polygram( Vec2(),         16, 4, 0x6666ffff, M_PI / 4.0f, ENEMY | VULNERABLE ) );
    AddShape( new Polygon ( Vec2( 16,  0 ), 8,  4, 0x6666ffff, M_PI / 4.0f ) );
    AddShape( new Polygon ( Vec2( -16, 0 ), 8,  4, 0x6666ffff, M_PI / 4.0f ) );
    AddShape( new Polygon ( Vec2( 0,  16 ), 8,  4, 0x6666ffff, M_PI / 4.0f ) );
    AddShape( new Polygon ( Vec2( 0, -16 ), 8,  4, 0x6666ffff, M_PI / 4.0f ) );
    SetScore( 50 );
    SetBoundingWidth( 16 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
}

void FollowHub::Update()
{
    _timer++;
    if ( _timer > TIMER ) {
        _timer = 0;
        _count++;
        Spawn( new Follow( GetPosition() ) );
        PlaySoundRandom( Lib::SOUND_ENEMY_SPAWN );
    }

    if ( GetPosition()._x < 0 )
        _dir.Set( 1, 0 );
    else if ( GetPosition()._x > Lib::WIDTH )
        _dir.Set( -1, 0 );
    else if ( GetPosition()._y < 0 )
        _dir.Set( 0, 1 );
    else if ( GetPosition()._y > Lib::HEIGHT )
        _dir.Set( 0, -1 );
    else if ( _count > 3 ) {
        _dir.Rotate( -M_PI / 2.0f );
        _count = 0;
    }

    Rotate( 0.05f );
    GetShape( 0 ).Rotate( -0.05f );

    Move( _dir * SPEED );
}

void FollowHub::OnDestroy( bool bomb )
{
    if ( !bomb )
        Spawn( new Chaser( GetPosition() ) );
}

// Shielder enemy
//------------------------------
Shielder::Shielder( const Vec2& position )
: Enemy( position, 16 )
, _dir( 0, 1 )
, _timer( 0 )
, _rDir( false )
{
    AddShape( new Polystar( Vec2(  24,  0 ), 8, 6, 0x006633ff, 0,           SHIELD ) );
    AddShape( new Polystar( Vec2( -24,  0 ), 8, 6, 0x006633ff, 0,           SHIELD ) );
    AddShape( new Polystar( Vec2( 0,   24 ), 8, 6, 0x006633ff, M_PI / 2.0f, SHIELD ) );
    AddShape( new Polystar( Vec2( 0,  -24 ), 8, 6, 0x006633ff, M_PI / 2.0f, SHIELD ) );
    AddShape( new Polygon ( Vec2(  24,  0 ), 8, 6, 0x33cc99ff, 0,           0 ) );
    AddShape( new Polygon ( Vec2( -24,  0 ), 8, 6, 0x33cc99ff, 0,           0 ) );
    AddShape( new Polygon ( Vec2( 0,   24 ), 8, 6, 0x33cc99ff, M_PI / 2.0f, 0 ) );
    AddShape( new Polygon ( Vec2( 0,  -24 ), 8, 6, 0x33cc99ff, M_PI / 2.0f, 0 ) );

    AddShape( new Polystar( Vec2( 0,    0 ), 24, 4, 0x006633ff, 0,           0 ) );
    AddShape( new Polygon ( Vec2( 0,    0 ), 14, 8, 0x006633ff, 0,           ENEMY | VULNERABLE ) );
    SetScore( 60 );
    SetBoundingWidth( 36 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
}

void Shielder::Update()
{
    Rotate( 0.04f );
    GetShape( 9 ).Rotate( -0.08f );
    for ( int i = 0; i < 8; i++ )
        GetShape( i ).Rotate( -0.04f );

    bool onScreen = false;
    if ( GetPosition()._x < 0 )
        _dir.Set( 1, 0 );
    else if ( GetPosition()._x > Lib::WIDTH )
        _dir.Set( -1, 0 );
    else if ( GetPosition()._y < 0 )
        _dir.Set( 0, 1 );
    else if ( GetPosition()._y > Lib::HEIGHT )
        _dir.Set( 0, -1 );
    else
        onScreen = true;

    if ( !onScreen && _rotate ) {
        _timer = 0;
        _rotate = false;
    }

    float speed = SPEED + 0.2f * ( 16 - GetHP() );
    if ( _rotate ) {
        Vec2 d( _dir );
        d.Rotate( ( _rDir ? 1 : -1 ) * ( TIMER - _timer ) * M_PI / ( 2.0f * float( TIMER ) ) );
        _timer--;
        if ( _timer <= 0 ) {
            _timer = 0;
            _rotate = false;
            _dir.Rotate( ( _rDir ? 1 : -1 ) * M_PI / 2.0f );
        }
        Move( d * speed );
    }
    else {
        _timer++;
        if ( _timer > TIMER * 2 ) {
            _timer = TIMER;
            _rotate = true;
            _rDir = GetLib().RandInt( 2 );
        }
        Move( _dir * speed );
    }
    _dir.Normalise();

}

// Tractor beam enemy
//------------------------------
Tractor::Tractor( const Vec2& position )
: Enemy( position, 50 )
, _timer( TIMER * 4 )
, _dir( 0, 0 )
, _ready( false )
, _spinning( false )
{
    AddShape( new Polygram( Vec2( 24,  0 ), 12, 6, 0xcc33ccff, 0,  ENEMY | VULNERABLE ) );
    AddShape( new Polygram( Vec2( -24, 0 ), 12, 6, 0xcc33ccff, 0,  ENEMY | VULNERABLE ) );
    AddShape( new Line    ( Vec2( 0,   0 ), Vec2( 24, 0 ), Vec2( -24, 0 ), 0xcc33ccff ) );
    SetScore( 85 );
    SetBoundingWidth( 36 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
}

void Tractor::Update()
{
    GetShape( 0 ).Rotate(  0.05f );
    GetShape( 1 ).Rotate( -0.05f );

    if ( GetPosition()._x < 0 )
        _dir.Set( 1, 0 );
    else if ( GetPosition()._x > Lib::WIDTH )
        _dir.Set( -1, 0 );
    else if ( GetPosition()._y < 0 )
        _dir.Set( 0, 1 );
    else if ( GetPosition()._y > Lib::HEIGHT )
        _dir.Set( 0, -1 );
    else
        _timer++;

    if ( !_ready && !_spinning ) {
        Move( _dir * SPEED * ( IsOnScreen() ? 1.0f : 2.5f ) );

        if ( _timer > TIMER * 8 ) {
            _ready = true;
            _timer = 0;
        }
    } else if ( _ready ) {
        if ( _timer > TIMER ) {
            _ready = false;
            _spinning = true;
            _timer = 0;
            _players = GetPlayers();
            PlaySound( Lib::SOUND_BOSS_FIRE );
        }
    } else if ( _spinning ) {
        Rotate( 0.3f );
        for ( unsigned int i = 0; i < _players.size(); i++ ) {
            if ( !( ( Player* )_players[ i ] )->IsKilled() ) {
                Vec2 d = GetPosition() - _players[ i ]->GetPosition();
                d.Normalise();
                _players[ i ]->Move( d * TRACTOR_SPEED );
            }
        }

        if ( _timer > TIMER * 5 ) {
            _spinning = false;
            _timer = 0;
        }
    }
}

void Tractor::Render()
{
    Enemy::Render();
    if ( _spinning ) {
        for ( unsigned int i = 0; i < _players.size(); i++ ) {
            if ( ( ( _timer + i * 4 ) / 4 ) % 2 && !( ( Player* )_players[ i ] )->IsKilled() ) {
                GetLib().RenderLine( GetPosition(), _players[ i ]->GetPosition(), 0xcc33ccff );
            }
        }
    }
}
