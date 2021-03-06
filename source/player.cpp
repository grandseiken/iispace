#include "player.h"
#include "lib.h"
#include "enemy.h"

const fixed Player::SPEED           = M_FIVE;
const fixed Player::SHOT_SPEED      = M_TEN;
const int   Player::SHOT_TIMER      = 4;
const fixed Player::BOMB_RADIUS     = M_ONE * 180;
const fixed Player::BOMB_BOSSRADIUS = M_ONE * 280;
const int   Player::BOMB_DAMAGE     = 50;
const int   Player::REVIVE_TIME     = 100;
const int   Player::SHIELD_TIME     = 50;
const int   Player::MAGICSHOT_COUNT = 120;

z0Game::ShipList Player::_killQueue;
z0Game::ShipList Player::_shotSoundQueue;
int              Player::_fireTimer;
Lib::Recording   Player::_replay;
std::size_t      Player::_replayFrame;

Player::Player( const Vec2& position, int playerNumber )
: Ship( position, false, true, false )
, _playerNumber( playerNumber )
, _score( 0 )
, _multiplier( 1 )
, _mulCount( 0 )
, _killTimer( 0 )
, _reviveTimer( REVIVE_TIME )
, _magicShotTimer( 0 )
, _shield( false )
, _bomb( false )
, _deathCounter( 0 )
{
    AddShape( new Polygon( Vec2(), 16, 3, GetPlayerColour() ) );
    AddShape( new Fill( Vec2( 8, 0 ), 2, 2, GetPlayerColour() ) );
    AddShape( new Fill( Vec2( 8, 0 ), 1, 1, GetPlayerColour() & 0xffffff33 ) );
    AddShape( new Fill( Vec2( 8, 0 ), 3, 3, GetPlayerColour() & 0xffffff33 ) );
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
    Vec2 velocity = lib.GetMoveVelocity( GetPlayerNumber() );
    Vec2 fireTarget = lib.GetFireTarget( GetPlayerNumber(), GetPosition() );
    int keys = int( lib.IsKeyHeld( GetPlayerNumber(), Lib::KEY_FIRE ) ) | ( lib.IsKeyPressed( GetPlayerNumber(), Lib::KEY_BOMB ) << 1 );
    if ( _replay._okay ) {
        if ( _replayFrame < _replay._playerFrames.size() ) {
            const Lib::PlayerFrame& pf = _replay._playerFrames[ _replayFrame ];
            velocity = pf._velocity;
            fireTarget = pf._target;
            keys = pf._keys;
            ++_replayFrame;
        }
        else {
            velocity = Vec2();
            fireTarget = _tempTarget;
            keys = 0;
        }
    }
    else
        lib.Record( velocity, fireTarget, keys );
    _tempTarget = fireTarget;

    if ( !( keys & 1 ) || _killTimer ) {
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
        if ( !_killTimer && !_killQueue.empty() ) {
            if ( GetLives() && _killQueue[ 0 ] == this ) {
                _killQueue.erase( _killQueue.begin() );
                _reviveTimer = REVIVE_TIME;
                Vec2 v( fixed( ( 1 + GetPlayerNumber() ) * Lib::WIDTH / ( 1 + CountPlayers() ) ), fixed( Lib::HEIGHT / 2 ) );
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
    Vec2 move = velocity;
    if ( move.Length() > M_PT_ZERO_ONE ) {

        if ( move.Length() > 1 )
            move.Normalise();
        move *= SPEED;

        Vec2 pos = GetPosition();
        pos += move;

        if ( pos._x < 0 )
            pos._x = 0;
        if ( pos._x > Lib::WIDTH )
            pos._x = Lib::WIDTH;
        if ( pos._y < 0 )
            pos._y = 0;
        if ( pos._y > Lib::HEIGHT )
            pos._y = Lib::HEIGHT;

        SetPosition( pos );
        SetRotation( move.Angle() );
    }

    // Bombs
    if ( _bomb && keys & 2 ) {
        _bomb = false;
        DestroyShape( 5 );

        Explosion( 0xffffffff, 16 );
        Explosion( GetPlayerColour(), 32 );
        Explosion( 0xffffffff, 48 );

        Vec2 t = GetPosition();
        Vec2f tf = Vec2f( t );
        for ( int i = 0; i < 64; ++i ) {
            Vec2 p;
            p.SetPolar( 2 * i * M_PI / 64, BOMB_RADIUS );
            p += t;
            SetPosition( p );
            Explosion( ( i % 2 ) ? GetPlayerColour() : 0xffffffff, 8 + GetLib().RandInt( 8 ) + GetLib().RandInt( 8 ), true, tf );
        }
        SetPosition( t );

        GetLib().Rumble( GetPlayerNumber(), 10 );
        PlaySound( Lib::SOUND_EXPLOSION );

        z0Game::ShipList list = GetShipsInRadius( GetPosition(), BOMB_BOSSRADIUS );
        for ( unsigned int i = 0; i < list.size(); i++ ) {
            if ( !list[ i ]->IsEnemy() )
                continue;
            if ( ( list[ i ]->GetPosition() - GetPosition() ).Length() <= BOMB_RADIUS || list[ i ]->IsBoss() )
                list[ i ]->Damage( BOMB_DAMAGE, false, 0 );
            if ( !list[ i ]->IsBoss() && ( ( Enemy* )list[ i ] )->GetScore() > 0 )
                AddScore( 0 );
        }
    }

    // Shots
    if ( !_fireTimer && keys & 1 ) {
        Vec2 v = fireTarget - GetPosition();
        if ( v.Length() > 0 ) {
            Spawn( new Shot( GetPosition(), this, v, _magicShotTimer != 0 ) );
            if ( _magicShotTimer )
                _magicShotTimer--;

            bool couldPlay = false;
            // Avoid randomness errors due to sound timings
            float volume = .5f * z_float( GetLib().RandFloat() ) + .5f;
            float pitch  = ( z_float( GetLib().RandFloat() ) - 1.f ) / 12.f;
            if ( _shotSoundQueue.empty() || _shotSoundQueue[ 0 ] == this ) {
                couldPlay = lib.PlaySound( Lib::SOUND_PLAYER_FIRE, volume, 2.f * z_float( GetPosition()._x ) / Lib::WIDTH - 1.f, pitch );
            }
            if ( couldPlay && !_shotSoundQueue.empty() ) {
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

void Player::Render() const
{
    const Lib& lib = GetLib();
    int n = GetPlayerNumber();

    if ( !_killTimer && ( !IsWhatMode() || _reviveTimer > 0 ) ) {
        Vec2f t = Vec2f( _tempTarget );
        if ( t._x >= 0 && t._x <= Lib::WIDTH && t._y >= 0 && t._y <= Lib::HEIGHT ) {
            lib.RenderLine( t + Vec2f( 0, 9 ), t - Vec2f( 0, 8 ), GetPlayerColour() );
            lib.RenderLine( t + Vec2f( 9, 1 ), t - Vec2f( 8, -1 ), GetPlayerColour() );
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

    Vec2f v;

    v.Set( 1.f + lib.LoadSettings()._hudCorrection, 1.f + lib.LoadSettings()._hudCorrection );
    if ( n == 1 )
        v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length() - lib.LoadSettings()._hudCorrection, 1.f + lib.LoadSettings()._hudCorrection );
    if ( n == 2 )
        v.Set( 1.f + lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f - lib.LoadSettings()._hudCorrection );
    if ( n == 3 )
        v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length() - lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f - lib.LoadSettings()._hudCorrection );

    lib.RenderText( v, s, z0Game::PANEL_TEXT );

    if ( _magicShotTimer != 0 ) {
        if ( n == 0 || n == 2 )
            v._x += int( s.length() );
        else
            v._x -= 1;
        v *= 16;
        lib.RenderRect( v + Vec2f( 5.f, 11.f - ( 10 * _magicShotTimer ) / MAGICSHOT_COUNT ), v + Vec2f( 9.f, 13.f ), 0xffffffff, 2 );
    }

    std::stringstream sss;
    if ( n % 2 == 1 )
        sss << _score << "   ";
    else
        sss << "   " << _score;
    s = sss.str();

    v.Set( 1.f + lib.LoadSettings()._hudCorrection, 1.f + lib.LoadSettings()._hudCorrection );
    if ( n == 1 )
        v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length() - lib.LoadSettings()._hudCorrection, 1.f + lib.LoadSettings()._hudCorrection );
    if ( n == 2 )
        v.Set( 1.f + lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f - lib.LoadSettings()._hudCorrection );
    if ( n == 3 )
        v.Set( Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length() - lib.LoadSettings()._hudCorrection, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f - lib.LoadSettings()._hudCorrection );

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
        DestroyShape( 5 );
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
    ++_deathCounter;
    if ( _shield || _bomb ) {
        DestroyShape( 5 );
        _shield = false;
        _bomb = false;
    }
    _killQueue.push_back( this );
    GetLib().Rumble( GetPlayerNumber(), 25 );
    PlaySound( Lib::SOUND_PLAYER_DESTROY );
}

static const int MULTIPLIER_lookup[ 24 ] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536,
    131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608
};

void Player::AddScore( long score )
{
    GetLib().Rumble( GetPlayerNumber(), 3 );
    _score += score * _multiplier;
    _mulCount++;
    if ( MULTIPLIER_lookup[ std::min( _multiplier + 3, 23 ) ] <= _mulCount ) {
        _mulCount = 0;
        _multiplier++;
    }
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
        DestroyShape( 5 );
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
        DestroyShape( 5 );
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
    AddShape( new Fill( Vec2(), M_TWO, M_TWO, _player->GetPlayerColour() ) );
    AddShape( new Fill( Vec2(), M_ONE, M_ONE, _player->GetPlayerColour() & 0xffffff33 ) );
    AddShape( new Fill( Vec2(), M_THREE, M_THREE, _player->GetPlayerColour() & 0xffffff33 ) );
}

void Shot::Render() const
{
    if ( IsWhatMode() )
        return;
    if ( _flash )
        RenderWithColour( 0xffffffff );
    else
        Ship::Render();
}

void Shot::Update()
{
    if ( _magic )
        _flash = GetLib().RandInt( 2 ) != 0;

    Move( _velocity );
    bool onScreen = GetPosition()._x >= -4 && GetPosition()._x < 4 + Lib::WIDTH &&
        GetPosition()._y >= -4 && GetPosition()._y < 4 + Lib::HEIGHT;
    if ( !onScreen ) {
        Destroy();
        return;
    }
    z0Game::ShipList kill = GetCollisionList( GetPosition(), VULNERABLE );
    for ( unsigned int i = 0; i < kill.size(); i++ ) {
        kill[ i ]->Damage( 1, _magic, _player );
        if ( !_magic )
            Destroy();
    }

    if ( AnyCollisionList( GetPosition(), SHIELD ) )
        Destroy();
    if ( !_magic && AnyCollisionList( GetPosition(), VULNSHIELD ) )
        Destroy();
}
