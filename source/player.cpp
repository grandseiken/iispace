#include "player.h"
#include "lib.h"
#include "enemy.h"

z0Game::ShipList Player::_killQueue;
z0Game::ShipList Player::_shotSoundQueue;
int              Player::_fireTimer;

Player::Player( const Vec2& position, int playerNumber )
: Ship( position )
, _playerNumber( playerNumber )
, _score( 0 )
, _multiplier( 1 )
, _mulCount( 0 )
, _killTimer( 0 )
, _reviveTimer( 0 )
, _magicShotTimer( 0 )
, _shield( false )
, _bomb( false )
{
    AddShape( new Polygon( Vec2(), 16, 3, GetPlayerColour() ) );
    AddShape( new Fill( Vec2( 8, 0 ), 2, 2, GetPlayerColour() ) );
    AddShape( new Polygon( Vec2(), 8,  3, GetPlayerColour(), M_PI ) );
    _killQueue.clear();
    _shotSoundQueue.clear();
    _fireTimer = 0;
}

Player::~Player()
{
}

void Player::Update()
{
    Lib& lib = GetLib();

    if ( !lib.IsKeyHeld( GetPlayerNumber(), Lib::KEY_FIRE ) || _killTimer ) {
        for ( unsigned int i = 0; i < _shotSoundQueue.size(); i++ ) {
            if ( _shotSoundQueue[ i ] == this ) {
                _shotSoundQueue.erase( _shotSoundQueue.begin() + i );
                break;
            }
        }
    }

    // Temporary death
    if ( _killTimer ) {
        _killTimer--;
        if ( !_killTimer && _killQueue.size() ) {
            if ( GetLives() && _killQueue[ 0 ] == this ) {
                _killQueue.erase( _killQueue.begin() );
                _reviveTimer = REVIVE_TIME;
                Vec2 v( ( 1 + GetPlayerNumber() ) * Lib::WIDTH / ( 1 + CountPlayers() ), Lib::HEIGHT / 2 );
                SetPosition( v );
                SubLife();
                lib.Rumble( GetPlayerNumber(), 10 );
                PlaySound( Lib::SOUND_PLAYER_RESPAWN );
            } else {
                _killTimer++;
            }
        }
        return;
    }
    if ( _reviveTimer )
        _reviveTimer--;

    // Movement
    Vec2 move = lib.GetMoveVelocity( GetPlayerNumber() );
    if ( move.Length() > 0.2f ) {

        if ( move.Length() > 1 ) move.Normalise();
        move *= SPEED;

        Vec2 pos = GetPosition();
        pos += move;

        if ( pos._x < 0 ) pos._x = 0;
        if ( pos._x > Lib::WIDTH ) pos._x = Lib::WIDTH;
        if ( pos._y < 0 ) pos._y = 0;
        if ( pos._y > Lib::HEIGHT ) pos._y = Lib::HEIGHT;

        SetPosition( pos );
        SetRotation( move.Angle() );
    }

    // Bombs
    if ( _bomb && lib.IsKeyPressed( GetPlayerNumber(), Lib::KEY_BOMB ) ) {
        _bomb = false;
        DestroyShape( 3 );

        Explosion( 0xffffffff, 12 );
        Explosion( GetPlayerColour(), 24 );
        Explosion( 0xffffffff, 36 );
        GetLib().Rumble( GetPlayerNumber(), 10 );
        PlaySound( Lib::SOUND_EXPLOSION );

        z0Game::ShipList list = GetShipsInRadius( GetPosition(), BOMB_BOSSRADIUS );
        for ( unsigned int i = 0; i < list.size(); i++ ) {
            if ( ( list[ i ]->GetPosition() - GetPosition() ).Length() <= BOMB_RADIUS || list[ i ]->IsBoss() )
                list[ i ]->Damage( BOMB_DAMAGE, 0 );
            AddScore( 0 );
        }
    }

    // Shots
    if ( !_fireTimer && lib.IsKeyHeld( GetPlayerNumber(), Lib::KEY_FIRE ) ) {
        Vec2 v = lib.GetFireTarget( GetPlayerNumber(), GetPosition() ) - GetPosition();
        if ( v.Length() > 0 ) {
            Spawn( new Shot( GetPosition(), this, v, _magicShotTimer ) );
            if ( _magicShotTimer )
                _magicShotTimer--;

            bool couldPlay = false;
            if ( !_shotSoundQueue.size() || _shotSoundQueue[ 0 ] == this ) {
                couldPlay = PlaySoundRandom( Lib::SOUND_PLAYER_FIRE, ( GetLib().RandFloat() - 1.0f ) / 12.0f );
            }
            if ( couldPlay && _shotSoundQueue.size() ) {
                _shotSoundQueue.erase( _shotSoundQueue.begin() );
            }
            if ( !couldPlay ) {
                bool inQueue = false;
                for ( unsigned int i = 0; i < _shotSoundQueue.size(); i++ ) {
                    if ( _shotSoundQueue[ i ] == this )
                        inQueue = true;
                }
                if ( !inQueue )
                    _shotSoundQueue.push_back( this );
            }
        }
    }
    
    // Damage
    if ( AnyCollisionList( GetPosition(), ENEMY ) )
        Damage();
}

void Player::Render()
{
    Lib& lib = GetLib();
    int n = GetPlayerNumber();

    if ( !_killTimer ) {
        Vec2 t = lib.GetFireTarget( n, GetPosition() );
        if ( t._x >= 0 && t._x <= Lib::WIDTH && t._y >= 0 && t._y <= Lib::HEIGHT ) {
            lib.RenderLine( t + Vec2( 0, 8 ), t - Vec2( 0, 8 ), GetPlayerColour() );
            lib.RenderLine( t + Vec2( 8, 0 ), t - Vec2( 8, 0 ), GetPlayerColour() );
        }
        if ( _reviveTimer % 2 )
            RenderWithColour( 0xffffffff );
        else
            Ship::Render();
    }

    if ( IsBossMode() )
        return;

    std::stringstream ss;
    ss << _multiplier << "X";
    std::string s = ss.str();

    Vec2 v;

    v.Set( 1 + lib.LoadSettings()._hudCorrection, 1 + lib.LoadSettings()._hudCorrection );
    if ( n == 1 ) v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1 - s.length() - lib.LoadSettings()._hudCorrection, 1 + lib.LoadSettings()._hudCorrection );
    if ( n == 2 ) v.Set( 1 + lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2 - lib.LoadSettings()._hudCorrection );
    if ( n == 3 ) v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1 - s.length() - lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2 - lib.LoadSettings()._hudCorrection );

    lib.RenderText( v, s, z0Game::PANEL_TEXT );

    std::stringstream sss;
    if ( n % 2 == 1 )
        sss << _score << "   ";
    else
        sss << "   " << _score;
    s = sss.str();

    v.Set( 1 + lib.LoadSettings()._hudCorrection, 1 + lib.LoadSettings()._hudCorrection );
    if ( n == 1 ) v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1 - s.length() - lib.LoadSettings()._hudCorrection, 1 + lib.LoadSettings()._hudCorrection );
    if ( n == 2 ) v.Set( 1 + lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2 - lib.LoadSettings()._hudCorrection );
    if ( n == 3 ) v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1 - s.length() - lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2 - lib.LoadSettings()._hudCorrection );

    lib.RenderText( v, s, GetPlayerColour() );
}

void Player::Damage()
{
    if ( _killTimer || _reviveTimer )
        return;
    for ( unsigned int i = 0; i < _killQueue.size(); i++ )
        if ( _killQueue[ i ] == this )
            return;
    if ( _shield ) {
        GetLib().Rumble( GetPlayerNumber(), 10 );
        PlaySound( Lib::SOUND_PLAYER_SHIELD );
        DestroyShape( 3 );
        _shield = false;

        _reviveTimer = SHIELD_TIME;
        return;
    }

    Explosion();
    Explosion( 0xffffffff, 14 );
    Explosion( 0, 20 );

    _magicShotTimer = 0;
    _multiplier = 1;
    _mulCount   = 0;
    _killTimer  = REVIVE_TIME;
    if ( _shield || _bomb ) {
        DestroyShape( 3 );
        _shield = false;
        _bomb = false;
    }
    _killQueue.push_back( this );
    GetLib().Rumble( GetPlayerNumber(), 25 );
    PlaySound( Lib::SOUND_PLAYER_DESTROY );
}

void Player::AddScore( long score )
{
    _score += score * ( score < 1000 ? _multiplier : 1 );
    _mulCount++;
    if ( _mulCount >= pow( 2.0f, _multiplier + 3 ) ) {
        _mulCount = 0;
        _multiplier++;
    }
    GetLib().Rumble( GetPlayerNumber(), 3 );
}

Colour Player::GetPlayerColour( int playerNumber )
{
    return playerNumber == 0 ? 0xff0000ff :
           playerNumber == 1 ? 0xff5500ff :
           playerNumber == 2 ? 0xffaa00ff :
           playerNumber == 3 ? 0xffff00ff :
                               0x00ff00ff ;
}

void Player::ActivateMagicShots()
{
    _magicShotTimer = MAGICSHOT_COUNT;
}

void Player::ActivateMagicShield()
{
    if ( _shield )
        return;
    if ( _bomb ) {
        DestroyShape( 3 );
        _bomb = false;
    }
    _shield = true;
    AddShape( new Polygon( Vec2(), 16, 10, 0xffffffff ) );
}

void Player::ActivateBomb()
{
    if ( _bomb )
        return;
    if ( _shield ) {
        DestroyShape( 3 );
        _shield = false;
    }
    _bomb = true;
    AddShape( new Polystar( Vec2( -8, 0 ), 6, 5, 0xffffffff, M_PI ) );
}

// Player projectiles
//------------------------------
Shot::Shot( const Vec2& position, Player* player, const Vec2& direction, bool magic )
: Ship( position ), _player( player ), _velocity( direction ), _magic( magic ), _flash( false )
{
    _velocity.Normalise();
    _velocity *= Player::SHOT_SPEED;
    AddShape( new Fill( Vec2(), 2.f, 2.f, _player->GetPlayerColour() ) );
}

void Shot::Render()
{
    if ( _flash )
        RenderWithColour( 0xffffffff );
    else
        Ship::Render();
}

void Shot::Update()
{
    if ( _magic )
        _flash = !_flash;

    Move( _velocity );
    Vec2 p = GetPosition();
    if ( !IsOnScreen() ) {
        Destroy();
        return;
    }
    z0Game::ShipList kill = GetCollisionList( GetPosition(), VULNERABLE );
    for ( unsigned int i = 0; i < kill.size(); i++ ) {
        kill[ i ]->Damage( 1, _player );
        if ( !_magic )
            Destroy();
    }

    if ( AnyCollisionList( GetPosition(), SHIELD ) )
        Destroy();
}
