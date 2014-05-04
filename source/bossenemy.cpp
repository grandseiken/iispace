#include "bossenemy.h"
#include "boss.h"
#include "player.h"

// Big follower
//------------------------------
BigFollow::BigFollow( const Vec2& position )
: Follow( position, 20, 3 )
{
    SetScore( 0 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
}

void BigFollow::OnDestroy( bool bomb )
{
    if ( bomb )
        return;

    Vec2 d( 10, 0 );
    d.Rotate( GetRotation() );
    for ( int i = 0; i < 3; i++ ) {
        Follow* s = new Follow( GetPosition() + d );
        s->SetScore( 0 );
        Spawn( s );
        d.Rotate( 2.0f * M_PI / 3.0f );
    }
}

// Generic boss projectile
//------------------------------
SBBossShot::SBBossShot( const Vec2& position, const Vec2& velocity, Colour c )
: Enemy( position, 0 )
, _dir( velocity )
{
    AddShape( new Polystar( Vec2(), 16, 8, c, 0, 0 ) );
    AddShape( new Polygon ( Vec2(), 10, 8, c, 0, ENEMY ) );
    SetBoundingWidth( 10 );
    SetScore( 0 );
}

void SBBossShot::Update()
{
    Move( _dir );
    Vec2 p = GetPosition();
    if ( p._x < -10 || p._x > Lib::WIDTH + 10 || p._y < -10 || p._y > Lib::HEIGHT + 10 )
        Destroy();
    SetRotation( GetRotation() + 0.02f );
}

// Tractor beam minion
//------------------------------
TBossShot::TBossShot( const Vec2& position, float angle )
: Enemy( position, 1 )
{
    AddShape( new Polygon( Vec2(), 8, 6, 0xcc33ccff, 0, ENEMY | VULNERABLE ) );
    _dir.SetPolar( angle, 3 );
    SetBoundingWidth( 8 );
    SetScore( 0 );
    SetDestroySound( Lib::SOUND_ENEMY_SHATTER );
}

void TBossShot::Update()
{
    if ( ( GetPosition()._x > Lib::WIDTH && _dir._x > 0 ) || ( GetPosition()._x < 0 && _dir._x < 0 ) )
        _dir._x = -_dir._x;
    if ( ( GetPosition()._y > Lib::HEIGHT && _dir._y > 0 ) || ( GetPosition()._y < 0 && _dir._y < 0 ) )
        _dir._y = -_dir._y;

    Move( _dir );
}

// Ghost boss wall
//------------------------------
GhostWall::GhostWall( bool swap, bool noGap, bool ignored )
: Enemy( Vec2( Lib::WIDTH / 2.0f, swap ? -10 : ( Lib::HEIGHT + 10 ) ), 0 )
, _dir( 0, swap ? 1 : -1 )
{
    if ( noGap ) {
        AddShape( new Box( Vec2(), Lib::WIDTH, 10.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );
        AddShape( new Box( Vec2(), Lib::WIDTH, 7.0f, 0xcc66ffff, 0, 0 ) );
        AddShape( new Box( Vec2(), Lib::WIDTH, 4.0f, 0xcc66ffff, 0, 0 ) );
    } else {
        AddShape( new Box( Vec2(), Lib::HEIGHT / 2.0f, 10.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );
        AddShape( new Box( Vec2(), Lib::HEIGHT / 2.0f, 7.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );
        AddShape( new Box( Vec2(), Lib::HEIGHT / 2.0f, 4.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );
    }
    SetBoundingWidth( Lib::WIDTH );
}

GhostWall::GhostWall( bool swap, bool swapGap )
: Enemy( Vec2( swap ? -10 : ( Lib::WIDTH + 10 ), Lib::HEIGHT / 2.0f ), 0 )
, _dir( swap ? 1 : -1, 0 )
{
    SetBoundingWidth( Lib::HEIGHT );
    float off = swapGap ? -100.0f : 100.0f;

    AddShape( new Box( Vec2( 0, off - 20 - Lib::HEIGHT / 2.0f ), 10.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );
    AddShape( new Box( Vec2( 0, off + 20 + Lib::HEIGHT / 2.0f ), 10.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, ENEMY | SHIELD ) );

    AddShape( new Box( Vec2( 0, off - 20 - Lib::HEIGHT / 2.0f ), 7.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, 0 ) );
    AddShape( new Box( Vec2( 0, off + 20 + Lib::HEIGHT / 2.0f ), 7.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, 0 ) );
    
    AddShape( new Box( Vec2( 0, off - 20 - Lib::HEIGHT / 2.0f ), 4.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, 0 ) );
    AddShape( new Box( Vec2( 0, off + 20 + Lib::HEIGHT / 2.0f ), 4.0f, Lib::HEIGHT / 2.0f, 0xcc66ffff, 0, 0 ) );
}

void GhostWall::Update()
{
    Move( _dir * SPEED );
    if ( _dir._x > 0 && GetPosition()._x > Lib::WIDTH  + 10 ||
         _dir._y > 0 && GetPosition()._y > Lib::HEIGHT + 10 ||
         _dir._x < 0 && GetPosition()._x < -10              ||
         _dir._y < 0 && GetPosition()._y < -10              ) {
         Destroy();
    }
}

// Ghost boss mine
//------------------------------
GhostMine::GhostMine( const Vec2& position, Ship* ghost )
: Enemy( position, 0 )
, _timer( 80 )
, _ghost( ghost )
{
    AddShape( new Polygon( Vec2(), 24, 8, 0x9933ccff, 0, 0 ) );
    SetBoundingWidth( 24 );
}

void GhostMine::Update()
{
    if ( _timer == 80 ) {
        Explosion();
        SetRotation( GetLib().RandFloat() * 2.0f * M_PI );
    }
    if ( _timer ) {
        _timer--;
        if ( !_timer )
            GetShape( 0 ).SetCategory( ENEMY );
    }
    z0Game::ShipList s = GetCollisionList( GetPosition(), ENEMY );
    for ( unsigned int i = 0; i < s.size(); i++ ) {
        if ( s[ i ] == _ghost ) {
            Spawn( new Follow( GetPosition() ) );
            Damage( 1, 0 );
            break;
        }
    }
}

void GhostMine::Render()
{
    if ( !( ( _timer / 4 ) % 2 ) )
        Enemy::Render();
}

// Death ray
//------------------------------
DeathRay::DeathRay( const Vec2& position )
: Enemy( position, 0 )
{
    AddShape( new Box ( Vec2(), 10, 48, 0,          0, ENEMY ) );
    AddShape( new Line( Vec2(), Vec2( 0, -48 ), Vec2( 0, 48 ), 0xffffffff, 0 ) );
    SetBoundingWidth( 48 );
}

void DeathRay::Update()
{
    Move( Vec2( 1, 0 ) * SPEED );
    if ( GetPosition()._x > Lib::WIDTH + 20 )
        Destroy();
}

// Death arm
//------------------------------
DeathArm::DeathArm( DeathRayBoss* boss, bool top, int hp )
: Enemy( Vec2(), hp )
, _boss( boss )
, _top( top )
, _timer( top ? 2 * DeathRayBoss::ARM_ATIMER / 3 : 0 )
, _attacking( false )
, _dir()
, _start( 30 )
{
    AddShape( new Polygon ( Vec2(), 60, 4, 0x33ff99ff, 0, 0 ) );
    AddShape( new Polygram( Vec2(), 50, 4, 0x33ff99ff, 0, VULNERABLE ) );
    AddShape( new Polygon ( Vec2(), 40, 4, 0,          0, SHIELD ) );
    SetBoundingWidth( 60 );
    SetDestroySound( Lib::SOUND_PLAYER_DESTROY );
}

void DeathArm::Update()
{
    Rotate( 0.05f );
    if ( _attacking ) {
        _timer++;
        if ( _timer < DeathRayBoss::ARM_ATIMER / 4 ) {
            Player* p = GetNearestPlayer();
            Vec2 d = p->GetPosition() - GetPosition();
            if ( d.Length() ) {
                _dir = d;
                _dir.Normalise();
                Move( _dir * DeathRayBoss::ARM_SPEED );
            }
        }
        else if ( _timer < DeathRayBoss::ARM_ATIMER / 2 ) {
            Move( _dir * DeathRayBoss::ARM_SPEED );
        }
        else if ( _timer < DeathRayBoss::ARM_ATIMER ) {
            Vec2 d = _boss->GetPosition() + Vec2( 80, _top ? 80 : -80 ) - GetPosition();
            if ( d.Length() > DeathRayBoss::ARM_SPEED ) {
                d.Normalise();
                Move( d * DeathRayBoss::ARM_SPEED );
            } 
            else {
                _attacking = false;
                _timer = 0;
            }
        }
        else if ( _timer >= DeathRayBoss::ARM_ATIMER ) {
            _attacking = false;
            _timer = 0;
        }
        return;
    }

    _timer++;
    if ( _timer >= DeathRayBoss::ARM_ATIMER ) {
        _timer = 0;
        _attacking = true;
        _dir.Set( 0, 0 );
        PlaySound( Lib::SOUND_BOSS_ATTACK );
    }
    SetPosition( _boss->GetPosition() + Vec2( 80, _top ? 80 : -80 ) );

    if ( _start ) {
        if ( _start == 30 ) {
            Explosion();
            Explosion( 0xffffffff );
        }
        _start--;
        if ( !_start )
            GetShape( 1 ).SetCategory( ENEMY | VULNERABLE );
    }
}

void DeathArm::OnDestroy( bool bomb )
{
    _boss->OnArmDeath( this );
}
