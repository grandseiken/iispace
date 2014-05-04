#include "powerup.h"
#include "player.h"

// Basic powerup
//------------------------------
Powerup::Powerup( const Vec2& position )
: Ship( position )
, _frame( 0 )
, _dir( 0, 1 )
, _rotate( false )
, _firstFrame( true )
{
    AddShape( new Polygon( Vec2(), 13, 5, 0, M_PI / 2.0f, 0 ) );
    AddShape( new Polygon( Vec2(), 9,  5, 0, M_PI / 2.0f, 0 ) );
}

void Powerup::Update()
{
    GetShape( 0 ).SetColour( Player::GetPlayerColour( _frame / 2 ) );
    _frame = ( _frame + 1 ) % ( Lib::PLAYERS * 2 );
    GetShape( 1 ).SetColour( Player::GetPlayerColour( _frame / 2 ) );

    if ( !IsOnScreen() ) {
        _dir = GetScreenCentre() - GetPosition();
    } else {
        if ( _firstFrame )
            _dir.SetPolar( GetLib().RandFloat() * 2.0f * M_PI, 1 );

        _dir.Rotate( 0.02f * ( _rotate ? 1 : -1 ) );
        if ( !GetLib().RandInt( TIME ) )
            _rotate = !_rotate;
    }
    _firstFrame = false;

    Player* p = GetNearestPlayer();
    bool alive = !p->IsKilled();
    Vec2 pv = p->GetPosition() - GetPosition();
    if ( pv.Length() <= 40 && alive ) {
        _dir = pv;
    }
    _dir.Normalise();
    Move( _dir * SPEED * ( ( pv.Length() <= 40 ) ? 3.0f : 1.0f ) );
    if ( pv.Length() <= 10 && alive ) {
        Damage( 1, ( Player* )p );
    }
}

void Powerup::Damage( int damage, Player* source )
{
    if ( source ) {
        OnGet( source );
        GetLib().Rumble( source->GetPlayerNumber(), 6 );
    }
    int r = 5 + GetLib().RandInt( 5 );
    for ( int i = 0; i < r; i++ ) {
        Vec2 dir;
        dir.SetPolar( GetLib().RandFloat() * 2.0f * M_PI, 6.0f );
        Spawn( new Particle( GetPosition(), 0xffffffff, dir, 4 + GetLib().RandInt( 8 ) ) );
    }
    Destroy();
}

// Extra life
//------------------------------
ExtraLife::ExtraLife( const Vec2& position )
: Powerup( position )
{
    AddShape( new Polygon( Vec2(), 8, 3, 0xffffffff, M_PI / 2.0f ) );
}

void ExtraLife::OnGet( Player* player )
{
    AddLife();
    PlaySound( Lib::SOUND_POWERUP_LIFE );
}

// Magic shots
//------------------------------
MagicShots::MagicShots( const Vec2& position )
: Powerup( position )
{
    AddShape( new Fill( Vec2(), 3, 3, 0xffffffff ) );
}

void MagicShots::OnGet( Player* player )
{
    player->ActivateMagicShots();
    PlaySound( Lib::SOUND_POWERUP_OTHER );
}

// Magic shield
//------------------------------
MagicShield::MagicShield( const Vec2& position )
: Powerup( position )
{
    AddShape( new Polygon( Vec2(), 11, 5, 0xffffffff, M_PI / 2.0f ) );
}

void MagicShield::OnGet( Player* player )
{
    player->ActivateMagicShield();
    PlaySound( Lib::SOUND_POWERUP_OTHER );
}

// Bomb
//------------------------------
Bomb::Bomb( const Vec2& position )
: Powerup( position )
{
    AddShape( new Polystar( Vec2(), 11, 10, 0xffffffff, M_PI / 2.0f ) );
}

void Bomb::OnGet( Player* player )
{
    player->ActivateBomb();
    PlaySound( Lib::SOUND_POWERUP_OTHER );
}
