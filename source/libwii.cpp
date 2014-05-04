#include <iostream>
#include <time.h>
#include <fat.h>
#include <fstream.h>
#include <asndlib.h>

#include "libwii.h"
#include "_Console.h"

LibWii::ExitType LibWii::_exitType;

LibWii::LibWii()
{
    _exitType = NO_EXIT;
}

LibWii::~LibWii()
{
    fatUnmount( "sd" );

    for ( unsigned int i = 0; i < 16; i++ )
        ASND_StopVoice( i );
    ASND_Pause( true );
    ASND_End();

    for ( unsigned int i = 0; i < _padIR.size(); i++ )
        delete _padIR[ i ];
    for ( unsigned int i = 0; i < _expansion.size(); i++ )
        delete _expansion[ i ];

    GRRLIB_Exit();
    free( _consoleFont.data );
}

// General
//------------------------------
void LibWii::Init()
{
    GRRLIB_Init();
    ClearScreen();
    GRRLIB_Render();

    _consoleFont = GRRLIB_LoadTexture( console );
    GRRLIB_InitTileSet( &_consoleFont, TEXT_WIDTH, TEXT_HEIGHT, 0 );

    PAD_Init();
    WPAD_Init();

    WPAD_SetDataFormat( WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR );
    WPAD_SetDataFormat( WPAD_CHAN_1, WPAD_FMT_BTNS_ACC_IR );
    WPAD_SetDataFormat( WPAD_CHAN_2, WPAD_FMT_BTNS_ACC_IR );
    WPAD_SetDataFormat( WPAD_CHAN_3, WPAD_FMT_BTNS_ACC_IR );

    for ( int i = 0; i < PLAYERS; i++ ) {
        WPAD_SetVRes( i, WIDTH, HEIGHT );
        _padIR.push_back( new PadIR() );
        _expansion.push_back( new Expansion() );
        _lastSubStick.push_back( Vec2( 0, -1.f ) );
        _rumble.push_back( 0 );
    }

    SYS_SetResetCallback( &OnReset );
    SYS_SetPowerCallback( &OnPower );
    WPAD_SetPowerButtonCallback( &OnPadPower );

    srand( ( unsigned int )time( 0 ) );
    fatInitDefault();

    LoadSounds();
    ASND_Init();
    ASND_Pause( false );

    ClearScreen();
    GRRLIB_Render();
}

void LibWii::BeginFrame()
{
    PAD_ScanPads();
    WPAD_ScanPads();
    for ( int i = 0; i < PLAYERS; i++ ) {
        WPAD_IR( i, _padIR[ i ] );
        WPAD_Expansion( i, _expansion[ i ] );
    }
    for ( unsigned int i = 0; i < _sounds.size(); i++ ) {
        if ( _sounds[ i ].first > 0 )
            _sounds[ i ].first--;
    }
}

void LibWii::EndFrame()
{
    GRRLIB_Render();
    for ( int i = 0; i < PLAYERS; i++ ) {
        if ( _rumble[ i ] ) {
            _rumble[ i ]--;
            if ( !_rumble[ i ] ) {
                WPAD_Rumble( i, false );
                PAD_ControlMotor( i, PAD_MOTOR_STOP );
            }
        }
    }
}

void LibWii::Exit( ExitType t )
{
    _exitType = t;
}

Lib::ExitType LibWii::GetExitType() const
{
    return _exitType;
}

int LibWii::RandInt( int lessThan ) const
{
    return rand() % lessThan;
}

float LibWii::RandFloat() const
{
    return float( rand() ) / float( RAND_MAX );
}

// SD card I/O
//------------------------------
Lib::SaveData LibWii::LoadSaveData()
{
    HighScoreTable r;
    for ( int i = 0; i < PLAYERS + 1; i++ ) {
        r.push_back( HighScoreList() );
        for ( unsigned int j = 0; j < ( i < PLAYERS ? NUM_HIGH_SCORES : PLAYERS ); j++ )
            r[ i ].push_back( HighScore() );
    }

    std::ifstream file;
    file.open( SAVE_PATH );

    if ( !file ) {
        SaveData data;
        data._bossesKilled = 0;
        data._highScores = r;
        return data;
    }
    std::string line;
    std::string in;
    while ( getline( file, line ) ) {
        in += line + "\n";
    }
    std::stringstream decrypted( Crypt( in ) );

    getline( decrypted, line );
    if ( line.compare( "WiiSPACE v1.0" ) != 0 ) {
        SaveData data;
        data._bossesKilled = 0;
        data._highScores = r;
        file.close();
        return data;
    }

    int bosses = 0;
    long total = 0;
    getline( decrypted, line );
    std::stringstream ssb( line );
    ssb >> bosses;
    ssb >> total;

    long findTotal = 0;
    int i = 0;
    unsigned int j = 0;
    while ( getline( decrypted, line ) ) {
        unsigned int split = line.find( ' ' );
        std::stringstream ss( line.substr( 0, split ) );
        std::string name;
        if ( line.length() > split + 1 )
            name = line.substr( split + 1 );
        long score;
        ss >> score;

        r[ i ][ j ].first = name;
        r[ i ][ j ].second = score;

        findTotal += score;

        j++;
        if ( j >= NUM_HIGH_SCORES || ( i >= PLAYERS && int( j ) >= PLAYERS ) ) {
            j -= NUM_HIGH_SCORES;
            i++;
        }
        if ( i >= PLAYERS + 1 )
            break;
    }

    file.close();

    if ( findTotal != total ) {
        r.clear();
        for ( int i = 0; i < PLAYERS + 1; i++ ) {
            r.push_back( HighScoreList() );
            for ( unsigned int j = 0; j < ( i < PLAYERS ? NUM_HIGH_SCORES : PLAYERS ); j++ )
                r[ i ].push_back( HighScore() );
        }
    }

    for ( int i = 0; i < PLAYERS; i++ ) {
        std::sort( r[ i ].begin(), r[ i ].end(), &ScoreSort );
    }
    
    SaveData data;
    data._bossesKilled = bosses;
    data._highScores = r;
    return data;
}

void LibWii::SaveSaveData( const SaveData& data )
{
    long total = 0;
    for ( int i = 0; i < PLAYERS + 1; i++ ) {
        for ( unsigned int j = 0; j < ( i < PLAYERS ? NUM_HIGH_SCORES : PLAYERS ); j++ ) {
            if ( i < int( data._highScores.size() ) && j < data._highScores[ i ].size() )
                total += data._highScores[ i ][ j ].second;
        }
    }

    std::stringstream out;
    out << "WiiSPACE v1.0\n" << data._bossesKilled << " " << total << "\n";

    for ( int i = 0; i < PLAYERS + 1; i++ ) {
        for ( unsigned int j = 0; j < ( i < PLAYERS ? NUM_HIGH_SCORES : PLAYERS ); j++ ) {
            if ( i < int( data._highScores.size() ) && j < data._highScores[ i ].size() )
                out << data._highScores[ i ][ j ].second << " " << data._highScores[ i ][ j ].first << "\n";
            else
                out << "0\n";
        }
    }

    std::string encrypted = Crypt( out.str() );

    std::ofstream file;
    file.open( SAVE_PATH );
    file << encrypted;
    file.close();
}

std::string LibWii::Crypt( const std::string& text )
{
    std::string r = "";
    std::string key( ENCRYPTION_KEY );
    for ( unsigned int i = 0; i < text.length(); i++ ) {
        r += text[ i ] ^ key[ i % key.length() ];
    }
    return r;
}

void LibWii::TakeScreenShot()
{
    GRRLIB_ScrShot( SCREENSHOT_PATH );
}

// Input
//------------------------------
bool LibWii::IsKeyPressed( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    u32 wDown = WPAD_ButtonsDown( player );
    u16 gDown  = PAD_ButtonsDown( player );

    return HasWKey( wDown, k ) || HasGKey( gDown, k );
}

bool LibWii::IsKeyReleased( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    u32 wUp = WPAD_ButtonsUp( player );
    u16 gUp  = PAD_ButtonsUp( player );

    return HasWKey( wUp, k ) || HasGKey( gUp, k );
}

bool LibWii::IsKeyHeld( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    if ( k == KEY_FIRE && PAD_TriggerR( player ) > 20 )
        return true;

    u32 wHeld = WPAD_ButtonsHeld( player );
    u16 gHeld  = PAD_ButtonsHeld( player );

    return HasWKey( wHeld, k ) || HasGKey( gHeld, k );
}

Vec2 LibWii::GetMoveVelocity ( int player )
{
    if ( player < 0 || player >= 4 ) return Vec2();
    Expansion& ex = *_expansion[ player ];

    bool kU = IsKeyHeld( player, KEY_UP );
    bool kD = IsKeyHeld( player, KEY_DOWN );
    bool kL = IsKeyHeld( player, KEY_LEFT );
    bool kR = IsKeyHeld( player, KEY_RIGHT );

    Vec2 v;
    if ( kU ) v += Vec2(  0, -1 );
    if ( kD ) v += Vec2(  0,  1 );
    if ( kL ) v += Vec2( -1,  0 );
    if ( kR ) v += Vec2(  1,  0 );
    if ( v.Length() > 0 )
        return v;

    Vec2 a;
    a.SetPolar( M_PI * ( ex.nunchuk.js.ang / 180.0f - 0.5f ), ex.nunchuk.js.mag );
    Vec2 b;
    b.Set( PAD_StickX( player ), -PAD_StickY( player ) );
    if ( b.Length() < 20.0f ) b.Set( 0, 0 );
    b /= 40.f;

    return a.Length() >= b.Length() ? a : b;
}

Vec2 LibWii::GetFireTarget( int player, const Vec2& position )
{
    if ( player < 0 || player >= 4 ) return Vec2();
    PadIR& p = *_padIR[ player ];
    if ( p.valid ) {
        Vec2 v( p.x, p.y );
        _lastSubStick[ player ] = v - position;
        return v;
    }
    Vec2 v( PAD_SubStickX( player ), -PAD_SubStickY( player ) );
    if ( v.Length() < 20.0f )
        v = _lastSubStick[ player ];
    else
        _lastSubStick[ player ] = v;
    v.Normalise();
    v *= 32.0f;
    return position + v;
}

// Output
//------------------------------
void LibWii::ClearScreen()
{
    GRRLIB_FillScreen( 0x000000ff );
}

void LibWii::RenderLine( const Vec2& a, const Vec2& b, Colour c )
{
    GRRLIB_Line( a._x, a._y, b._x, b._y, c );
}

void LibWii::RenderText( const Vec2& v, const std::string& text, Colour c )
{
    GRRLIB_Printf( v._x * TEXT_WIDTH, v._y * TEXT_HEIGHT, _consoleFont, c, 1, text.c_str() );
}

void LibWii::RenderRect( const Vec2& low, const Vec2& hi, Colour c, int lineWidth )
{
    GRRLIB_Rectangle( low._x, low._y, hi._x - low._x, hi._y - low._y, c, lineWidth == 0 );
    if ( lineWidth > 1 )
        RenderRect( low + Vec2( 1, 1 ), hi - Vec2( 1, 1 ), c, lineWidth - 1 );
}

void LibWii::Rumble( int player, int time )
{
    if ( player < 0 || player >= PLAYERS )
        return;
    _rumble[ player ] = std::max( _rumble[ player ], time );
    if ( _rumble[ player ] ) {
        WPAD_Rumble( player, true );
        PAD_ControlMotor( player, PAD_MOTOR_RUMBLE );
    }
}

void LibWii::StopRumble()
{
    for ( int i = 0; i < PLAYERS; i++ ) {
        _rumble[ i ] = 0;
        WPAD_Rumble( i, false );
        PAD_ControlMotor( i, PAD_MOTOR_STOP );
    }
}

bool LibWii::PlaySound( Sound sound, float volume, float pan, float repitch )
{
    if ( volume < 0.0f )
        volume = 0.0f;
    if ( volume > 1.0f )
        volume = 1.0f;
    if ( pan < -1.0f )
        pan = -1.0f;
    if ( pan > 1.0f )
        pan = 1.0f;

    int lVol = 255.0f * volume * ( 1.0f - pan ) / 2.0f;
    int rVol = 255.0f * volume * ( 1.0f + pan ) / 2.0f;

    u8* raw    = 0;
    s32 length = 0;

    for ( unsigned int i = 0; i < _sounds.size(); i++ ) {
        if ( sound == _sounds[ i ].second.first ) {
            if ( _sounds[ i ].first <= 0 ) {
                raw    = _sounds[ i ].second.second.first;
                length = _sounds[ i ].second.second.second;
                _sounds[ i ].first = SOUND_TIMER;
            }
            break;
        }
    }

    s32 voice = ASND_GetFirstUnusedVoice();
    if ( raw && voice != SND_INVALID ) {
        ASND_SetVoice( voice, VOICE_MONO_16BIT, 48000 * pow( 2, repitch ), 0, raw, length, lVol, rVol, 0 );
        return true;
    }
    return false;
}

// Internal functions
//------------------------------
void LibWii::OnReset()
{
    _exitType = EXIT_TO_LOADER;
}

void LibWii::OnPower()
{
    _exitType = EXIT_POWER_OFF;
}

void LibWii::OnPadPower( s32 c )
{
    OnPower();
}

// Button mappings
//------------------------------
bool LibWii::HasWKey( u32 wPad, Key k )
{
    if ( k == KEY_FIRE )
        return wPad & WPAD_BUTTON_B;
    else if ( k == KEY_BOMB )
        return wPad & WPAD_BUTTON_A || wPad & WPAD_NUNCHUK_BUTTON_Z;
    else if ( k == KEY_ACCEPT )
        return wPad & WPAD_BUTTON_A;
    else if ( k == KEY_CANCEL )
        return wPad & WPAD_BUTTON_B;
    else if ( k == KEY_MENU )
        return wPad & WPAD_BUTTON_MINUS || wPad & WPAD_BUTTON_PLUS || wPad & WPAD_BUTTON_HOME;
    else if ( k == KEY_UP )
        return wPad & WPAD_BUTTON_UP;
    else if ( k == KEY_DOWN )
        return wPad & WPAD_BUTTON_DOWN;
    else if ( k == KEY_LEFT )
        return wPad & WPAD_BUTTON_LEFT;
    else if ( k == KEY_RIGHT )
        return wPad & WPAD_BUTTON_RIGHT;

    return false;
}

bool LibWii::HasGKey( u16 gPad, Key k )
{
    if ( k == KEY_FIRE )
        return gPad & PAD_TRIGGER_R;
    else if ( k == KEY_BOMB )
        return gPad & PAD_TRIGGER_L || gPad & PAD_BUTTON_A;
    else if ( k == KEY_ACCEPT )
        return gPad & PAD_BUTTON_A || gPad & PAD_TRIGGER_R;
    else if ( k == KEY_CANCEL )
        return gPad & PAD_BUTTON_B || gPad & PAD_TRIGGER_L;
    else if ( k == KEY_MENU )
        return gPad & PAD_TRIGGER_Z || gPad & PAD_BUTTON_START;
    else if ( k == KEY_UP )
        return gPad & PAD_BUTTON_UP;
    else if ( k == KEY_DOWN )
        return gPad & PAD_BUTTON_DOWN;
    else if ( k == KEY_LEFT )
        return gPad & PAD_BUTTON_LEFT;
    else if ( k == KEY_RIGHT )
        return gPad & PAD_BUTTON_RIGHT;

    return false;
}

// Sounds
//------------------------------
#include "_MenuClick.h"
#include "_MenuAccept.h"
#include "_PowerupLife.h"
#include "_PowerupOther.h"
#include "_EnemyHit.h"
#include "_EnemyDestroy.h"
#include "_EnemyShatter.h"
#include "_PlayerFire.h"
#include "_PlayerDestroy.h"
#include "_PlayerRespawn.h"
#include "_PlayerShield.h"
#include "_Explosion.h"
#include "_BossAttack.h"
#include "_BossFire.h"
#include "_EnemySpawn.h"

void LibWii::LoadSounds()
{
    // PCM mono unsigned 16-bit big-endian 48khz
    USE_SOUND( SOUND_MENU_CLICK,     MenuClickPCM     );
    USE_SOUND( SOUND_MENU_ACCEPT,    MenuAcceptPCM    );
    USE_SOUND( SOUND_POWERUP_LIFE,   PowerupLifePCM   );
    USE_SOUND( SOUND_POWERUP_OTHER,  PowerupOtherPCM  );
    USE_SOUND( SOUND_ENEMY_HIT,      EnemyHitPCM      );
    USE_SOUND( SOUND_ENEMY_DESTROY,  EnemyDestroyPCM  );
    USE_SOUND( SOUND_ENEMY_SHATTER,  EnemyShatterPCM  );
    USE_SOUND( SOUND_ENEMY_SPAWN,    EnemySpawnPCM    );
    USE_SOUND( SOUND_BOSS_ATTACK,    BossAttackPCM    );
    USE_SOUND( SOUND_BOSS_FIRE,      BossFirePCM      );
    USE_SOUND( SOUND_PLAYER_FIRE,    PlayerFirePCM    );
    USE_SOUND( SOUND_PLAYER_RESPAWN, PlayerRespawnPCM );
    USE_SOUND( SOUND_PLAYER_DESTROY, PlayerDestroyPCM );
    USE_SOUND( SOUND_PLAYER_SHIELD,  PlayerShieldPCM  );
    USE_SOUND( SOUND_EXPLOSION,      ExplosionPCM     );
}
