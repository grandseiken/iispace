#include "enemy.h"
#include "bossenemy.h"
#include "player.h"

const int   Follow::TIME = 90;
const fixed Follow::SPEED = M_TWO;
const int   Chaser::TIME = 60;
const fixed Chaser::SPEED = M_FOUR;
const fixed Square::SPEED = M_TWO + M_HALF / 2;
const int   Wall::TIMER = 80;
const fixed Wall::SPEED = M_ONE + M_HALF / 2;
const int   FollowHub::TIMER = 170;
const fixed FollowHub::SPEED = M_ONE;
const int   Shielder::TIMER = 80;
const fixed Shielder::SPEED = M_TWO;
const int   Tractor::TIMER = 50;
const fixed Tractor::SPEED = 6 * M_PT_ONE;
const fixed Tractor::TRACTOR_SPEED = M_TWO + M_HALF;

// Basic enemy
//------------------------------
Enemy::Enemy( const Vec2& position, int hp, bool explodeOnDestroy )
: Ship( position )
, _hp( hp )
, _score( 0 )
, _damaged( false )
, _destroySound( Lib::SOUND_ENEMY_DESTROY )
, _explodeOnDestroy( explodeOnDestroy )
{
}

void Enemy::Damage( int damage, bool magic, Player* source ) {
    _hp -= std::max( damage, 0 );
    if ( damage > 0 )
        PlaySoundRandom( Lib::SOUND_ENEMY_HIT );

    if ( _hp <= 0 && !IsDestroyed() ) {
        PlaySoundRandom( _destroySound );
        if ( source && GetScore() > 0 )
            source->AddScore( GetScore() );
        if ( _explodeOnDestroy )
            Explosion();
        else
            Explosion( 0, 4, true, Vec2f( GetPosition() ) );
        OnDestroy( damage >= Player::BOMB_DAMAGE );
        Destroy();
    } else if ( !IsDestroyed() ) {
        if ( damage > 0 )
            PlaySoundRandom( Lib::SOUND_ENEMY_HIT );
        _damaged = damage >= Player::BOMB_DAMAGE ? 25 : 1;
    }
}

void Enemy::Render() const
{
    if ( !_damaged ) {
        Ship::Render();
        return;
    }
    RenderWithColour( 0xffffffff );
    --_damaged;
}

// Follower enemy
//------------------------------
Follow::Follow( const Vec2& position, fixed radius, int hp )
: Enemy( position, hp )
, _timer( 0 )
, _target( 0 )
{
    AddShape( new Polygon( Vec2(), radius, 4, 0x9933ffff, 0, ENEMY | VULNERABLE ) );
    SetScore( 15 );
    SetBoundingWidth( 10 );
    SetDestroySound( Lib::SOUND_ENEMY_SHATTER );
    SetEnemyValue( 1 );
}

void Follow::Update()
{
    Rotate( M_PT_ONE );
    if ( Player::CountKilledPlayers() >= GetPlayers().size() )
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
    SetEnemyValue( 2 );
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
        Rotate( M_PT_ONE );
    }
}

// Square enemy
//------------------------------
Square::Square( const Vec2& position, fixed rotation )
: Enemy( position, 4 )
, _dir()
, _timer( z_rand() % 80 + 40 )
{
    AddShape( new Box( Vec2(), 10, 10, 0x33ff33ff, 0, ENEMY | VULNERABLE ) ); _dir.SetPolar( rotation, M_ONE );
    SetScore( 25 );
    SetBoundingWidth( 15 );
    SetEnemyValue( 2 );
}

void Square::Update()
{
    if ( GetNonWallCount() == 0 && IsOnScreen() )
        _timer--;
    else
        _timer = GetLib().RandInt( 80 ) + 40;

    if ( _timer == 0 )
        Damage( 4, false, 0 );

    Vec2 pos = GetPosition();
    if ( pos._x < 0 && _dir._x <= 0 ) {
        _dir._x = -_dir._x;
        if ( _dir._x <= 0 ) _dir._x = 1;
    }
    if ( pos._y < 0 && _dir._y <= 0 ) {
        _dir._y = -_dir._y;
        if ( _dir._y <= 0 ) _dir._y = 1;
    }

    if ( pos._x > Lib::WIDTH && _dir._x >= 0 ) {
        _dir._x = -_dir._x;
        if ( _dir._x >= 0 ) _dir._x = -1;
    }
    if ( pos._y > Lib::HEIGHT && _dir._y >= 0 ) {
        _dir._y = -_dir._y;
        if ( _dir._y >= 0 ) _dir._y = -1;
    }
    _dir.Normalise();

    Move( _dir * SPEED );
    SetRotation( _dir.Angle() );
}

void Square::Render() const
{
    if ( GetNonWallCount() == 0 && ( _timer % 4 == 1 || _timer % 4 == 2 ) )
        RenderWithColour( 0x33333300 );
    else
        Enemy::Render();
}

// Wall enemy
//------------------------------
Wall::Wall( const Vec2& position, bool rdir )
: Enemy( position, 10 )
, _dir( 0, 1 )
, _timer( 0 )
, _rotate( false )
, _rdir( rdir )
{
    AddShape( new Box( Vec2(), 10, 40, 0x33cc33ff, 0, ENEMY | VULNERABLE ) );
    SetScore( 20 );
    SetBoundingWidth( 50 );
    SetEnemyValue( 4 );
}

void Wall::Update()
{
    if ( GetNonWallCount() == 0 && _timer % 8 < 2 ) {
        if ( GetHP() > 2 )
            PlaySound( Lib::SOUND_ENEMY_SPAWN );
        Damage( GetHP() - 2, false, 0 );
    }

    if ( _rotate ) {
        Vec2 d( _dir );
        d.Rotate( ( _rdir ? _timer - TIMER : TIMER - _timer ) * M_PI / ( M_FOUR * TIMER ) );
        SetRotation( d.Angle() );
        _timer--;
        if ( _timer <= 0 ) {
            _timer = 0;
            _rotate = false;
            _dir.Rotate( _rdir ? -M_PI / M_FOUR : M_PI / M_FOUR );
        }
        return;
    }
    else {
        _timer++;
        if ( _timer > TIMER * 6 ) {
            if ( IsOnScreen() ) {
                _timer = TIMER;
                _rotate = true;
            }
            else
                _timer = 0;
        }
    }

    Vec2 pos = GetPosition();
    if ( ( pos._x < 0 && _dir._x < -M_PT_ZERO_ONE ) ||
         ( pos._y < 0 && _dir._y < -M_PT_ZERO_ONE ) ||
         ( pos._x > Lib::WIDTH && _dir._x > M_PT_ZERO_ONE ) ||
         ( pos._y > Lib::HEIGHT && _dir._y > M_PT_ZERO_ONE ) ) {
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
    d.Rotate( M_PI / M_TWO );
    Vec2 v = GetPosition() + d * M_TEN * 3;
    if ( v._x >= 0 && v._x <= Lib::WIDTH && v._y >= 0 && v._y <= Lib::HEIGHT )
        Spawn( new Square( v, GetRotation() ) );
    v = GetPosition() - d * M_TEN * 3;
    if ( v._x >= 0 && v._x <= Lib::WIDTH && v._y >= 0 && v._y <= Lib::HEIGHT )
        Spawn( new Square( v, GetRotation() ) );
}

// Follow spawner enemy
//------------------------------
FollowHub::FollowHub( const Vec2& position, bool powerA, bool powerB )
: Enemy( position, 14 )
, _timer( 0 )
, _dir( 0, 0 )
, _count( 0 )
, _powerA( powerA )
, _powerB( powerB )
{
    AddShape( new Polygram( Vec2(), 16, 4, 0x6666ffff, M_PI / M_FOUR, ENEMY | VULNERABLE ) );
    if ( _powerB ) {
        AddShape( new Polystar( Vec2( 16,  0 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
        AddShape( new Polystar( Vec2( -16, 0 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
        AddShape( new Polystar( Vec2( 0,  16 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
        AddShape( new Polystar( Vec2( 0, -16 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
    }
    AddShape( new Polygon( Vec2( 16,  0 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
    AddShape( new Polygon( Vec2( -16, 0 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
    AddShape( new Polygon( Vec2( 0,  16 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
    AddShape( new Polygon( Vec2( 0, -16 ), 8, 4, 0x6666ffff, M_PI / M_FOUR ) );
    SetScore( 50 + _powerA * 10 + _powerB * 10 );
    SetBoundingWidth( 16 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
    SetEnemyValue( 6 + ( powerA ? 2 : 0 ) + ( powerB ? 2 : 0 ) );
}

void FollowHub::Update()
{
    _timer++;
    if ( _timer > ( _powerA ? TIMER / 2 : TIMER ) ) {
        _timer = 0;
        _count++;
        if ( IsOnScreen() ) {
            if ( _powerB )
                Spawn( new Chaser( GetPosition() ) );
            else
                Spawn( new Follow( GetPosition() ) );
            PlaySoundRandom( Lib::SOUND_ENEMY_SPAWN );
        }
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
        _dir.Rotate( -M_PI / M_TWO );
        _count = 0;
    }

    fixed s = _powerA ? M_PT_ZERO_ONE * 5 + M_PT_ONE : M_PT_ZERO_ONE * 5;
    Rotate( s );
    GetShape( 0 ).Rotate( -s );

    Move( _dir * SPEED );
}

void FollowHub::OnDestroy( bool bomb )
{
    if ( bomb )
        return;
    if ( _powerB )
        Spawn( new BigFollow( GetPosition(), true ) );
    Spawn( new Chaser( GetPosition() ) );
}

// Shielder enemy
//------------------------------
Shielder::Shielder( const Vec2& position, bool power )
: Enemy( position, 16 )
, _dir( 0, 1 )
, _timer( 0 )
, _rotate( false )
, _rDir( false )
, _power( power )
{
    AddShape( new Polystar( Vec2(  24,  0 ), 8, 6, 0x006633ff, 0,            VULNSHIELD ) );
    AddShape( new Polystar( Vec2( -24,  0 ), 8, 6, 0x006633ff, 0,            VULNSHIELD ) );
    AddShape( new Polystar( Vec2( 0,   24 ), 8, 6, 0x006633ff, M_PI / M_TWO, VULNSHIELD ) );
    AddShape( new Polystar( Vec2( 0,  -24 ), 8, 6, 0x006633ff, M_PI / M_TWO, VULNSHIELD ) );
    AddShape( new Polygon ( Vec2(  24,  0 ), 8, 6, 0x33cc99ff, 0,            0 ) );
    AddShape( new Polygon ( Vec2( -24,  0 ), 8, 6, 0x33cc99ff, 0,            0 ) );
    AddShape( new Polygon ( Vec2( 0,   24 ), 8, 6, 0x33cc99ff, M_PI / M_TWO, 0 ) );
    AddShape( new Polygon ( Vec2( 0,  -24 ), 8, 6, 0x33cc99ff, M_PI / M_TWO, 0 ) );

    AddShape( new Polystar( Vec2( 0,    0 ), 24, 4,                      0x006633ff, 0, 0 ) );
    AddShape( new Polygon ( Vec2( 0,    0 ), 14, 8, power ? 0x33cc99ff : 0x006633ff, 0, ENEMY | VULNERABLE ) );
    SetScore( 60 + _power * 40 );
    SetBoundingWidth( 36 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
    SetEnemyValue( 8 + ( power ? 2 : 0 ) );
}

void Shielder::Update()
{
    fixed s = _power ? M_PT_ZERO_ONE * 12 : M_PT_ZERO_ONE * 4;
    Rotate( s );
    GetShape( 9 ).Rotate( -2 * s );
    for ( int i = 0; i < 8; i++ )
        GetShape( i ).Rotate( -s );

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

    fixed speed = SPEED + ( _power ? M_PT_ONE * 3 : M_PT_ONE * 2 ) * ( 16 - GetHP() );
    if ( _rotate ) {
        Vec2 d( _dir );
        d.Rotate( ( _rDir ? 1 : -1 ) * ( TIMER - _timer ) * M_PI / ( M_TWO * TIMER ) );
        _timer--;
        if ( _timer <= 0 ) {
            _timer = 0;
            _rotate = false;
            _dir.Rotate( ( _rDir ? 1 : -1 ) * M_PI / M_TWO );
        }
        Move( d * speed );
    }
    else {
        _timer++;
        if ( _timer > TIMER * 2 ) {
            _timer = TIMER;
            _rotate = true;
            _rDir = GetLib().RandInt( 2 ) != 0;
        }
        if ( IsOnScreen() && _timer % TIMER == TIMER / 2 && _power ) {
            Player* p = GetNearestPlayer();
            Vec2 v = GetPosition();

            Vec2 d = p->GetPosition() - v;
            d.Normalise();
            Spawn( new SBBossShot( v, d * M_THREE, 0x33cc99ff ) );
            PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
        }
        Move( _dir * speed );
    }
    _dir.Normalise();

}

// Tractor beam enemy
//------------------------------
Tractor::Tractor( const Vec2& position, bool power )
: Enemy( position, 50 )
, _timer( TIMER * 4 )
, _dir( 0, 0 )
, _power( power )
, _ready( false )
, _spinning( false )
{
    AddShape( new Polygram( Vec2( 24,  0 ), 12, 6, 0xcc33ccff, 0,  ENEMY | VULNERABLE ) );
    AddShape( new Polygram( Vec2( -24, 0 ), 12, 6, 0xcc33ccff, 0,  ENEMY | VULNERABLE ) );
    AddShape( new Line    ( Vec2( 0,   0 ), Vec2( 24, 0 ), Vec2( -24, 0 ), 0xcc33ccff ) );
    if ( power ) {
        AddShape( new Polystar( Vec2( 24,  0 ), 16, 6, 0xcc33ccff, 0, 0 ) );
        AddShape( new Polystar( Vec2( -24, 0 ), 16, 6, 0xcc33ccff, 0, 0 ) );
    }
    SetScore( 85 + power * 40 );
    SetBoundingWidth( 36 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
    SetEnemyValue( 10 + ( power ? 2 : 0 ) );
}

void Tractor::Update()
{
    GetShape( 0 ).Rotate(  M_PT_ZERO_ONE * 5 );
    GetShape( 1 ).Rotate( -M_PT_ZERO_ONE * 5 );
    if ( _power ) {
        GetShape( 3 ).Rotate( -M_PT_ZERO_ONE * 8 );
        GetShape( 4 ).Rotate(  M_PT_ZERO_ONE * 8 );
    }

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
        Move( _dir * SPEED * ( IsOnScreen() ? M_ONE : M_TWO + M_HALF ) );

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
        Rotate( M_PT_ONE * 3 );
        for ( unsigned int i = 0; i < _players.size(); i++ ) {
            if ( !( ( Player* )_players[ i ] )->IsKilled() ) {
                Vec2 d = GetPosition() - _players[ i ]->GetPosition();
                d.Normalise();
                _players[ i ]->Move( d * TRACTOR_SPEED );
            }
        }

        if ( _timer % ( TIMER / 2 ) == 0 && IsOnScreen() && _power ) {
            Player* p = GetNearestPlayer();
            Vec2 v = GetPosition();

            Vec2 d = p->GetPosition() - v;
            d.Normalise();
            Spawn( new SBBossShot( v, d * M_FOUR, 0xcc33ccff ) );
            PlaySoundRandom( Lib::SOUND_BOSS_FIRE );
        }

        if ( _timer > TIMER * 5 ) {
            _spinning = false;
            _timer = 0;
        }
    }
}

void Tractor::Render() const
{
    Enemy::Render();
    if ( _spinning ) {
        for ( unsigned int i = 0; i < _players.size(); i++ ) {
            if ( ( ( _timer + i * 4 ) / 4 ) % 2 && !( ( Player* )_players[ i ] )->IsKilled() ) {
                GetLib().RenderLine( Vec2f( GetPosition() ), Vec2f( _players[ i ]->GetPosition() ), 0xcc33ccff );
            }
        }
    }
}
