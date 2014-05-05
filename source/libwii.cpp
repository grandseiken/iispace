#include "z0.h"
#ifdef PLATFORM_WII

#include <iostream>
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
    _wConnectedPads = 0;
    _gConnectedPads = 0;
    _cConnectedPads = 0;
    _server = 0;

    SYS_SetResetCallback( &OnReset );
    SYS_SetPowerCallback( &OnPower );
    WPAD_SetPowerButtonCallback( &OnPadPower );

    fatInitDefault();

    LoadSounds();
    ASND_Init();
    ASND_Pause( false );

    _settings = LoadSaveSettings();

    ClearScreen();
    GRRLIB_Render();
}

void LibWii::BeginFrame()
{
    _gConnectedPads = PAD_ScanPads();
    _wConnectedPads = 0;
    _cConnectedPads = 0;
    WPAD_ScanPads();

    for ( int i = 0; i < PLAYERS; i++ ) {
        u32 type;
        if ( WPAD_Probe( i, &type ) == WPAD_ERR_NONE ) {
            _wConnectedPads |= ( 1 << i );
            if ( type == WPAD_EXP_CLASSIC )
                _cConnectedPads |= ( 1 << i );
        }
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

void LibWii::SystemExit( bool powerOff ) const
{
    SYS_ResetSystem( powerOff ? SYS_POWEROFF : SYS_RETURNTOMENU, 0, 0 );
}

int LibWii::RandInt( int lessThan )
{
    return z_rand() % lessThan;
}

fixed LibWii::RandFloat()
{
    return fixed( z_rand() ) / fixed( Z_RAND_MAX );
}

void LibWii::TakeScreenShot()
{
    GRRLIB_ScrShot( ScreenshotPath() );
}

LibWii::Settings LibWii::LoadSettings()
{
    return _settings;
}

// Input
//------------------------------
LibWii::PadType LibWii::IsPadConnected( int player )
{
    return PadType( ( _gConnectedPads & ( 1 << player ) ? PAD_GAMECUBE : PAD_NONE )
                  + ( _wConnectedPads & ( 1 << player ) ? PAD_WIIMOTE  : PAD_NONE )
                  + ( _cConnectedPads & ( 1 << player ) ? PAD_CLASSIC  : PAD_NONE ) );
}

bool LibWii::IsKeyPressed( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    u32 wDown  = WPAD_ButtonsDown( player );
    u16 gDown  = PAD_ButtonsDown( player );

    return ( _cConnectedPads & ( 1 << player ) ? HasCKey( wDown, k ) : HasNKey( wDown, k ) ) || HasWKey( wDown, k ) || HasGKey( gDown, k );
}

bool LibWii::IsKeyReleased( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    u32 wUp  = WPAD_ButtonsUp( player );
    u16 gUp  = PAD_ButtonsUp( player );

    return ( _cConnectedPads & ( 1 << player ) ? HasCKey( wUp, k ) : HasNKey( wUp, k ) ) || HasWKey( wUp, k ) || HasGKey( gUp, k );
}

bool LibWii::IsKeyHeld( int player, Key k )
{
    if ( player < 0 || player >= 4 ) return false;
    if ( k == KEY_FIRE && ( PAD_TriggerR( player ) > 20
                         || Vec2( PAD_SubStickX( player ), PAD_SubStickY( player ) ).Length() >= 20.0f
                         || _expansion[ player ]->classic.rjs.mag >= 0.5f ) )
        return true;

    u32 wHeld  = WPAD_ButtonsHeld( player );
    u16 gHeld  = PAD_ButtonsHeld( player );

    return ( _cConnectedPads & ( 1 << player ) ? HasCKey( wHeld, k ) : HasNKey( wHeld, k ) ) || HasWKey( wHeld, k ) || HasGKey( gHeld, k );
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
    if ( _cConnectedPads & ( 1 << player ) )
        a.SetPolar( M_PI * ( ex.classic.ljs.ang / 180.0f - 0.5f ), ex.classic.ljs.mag );
    else
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
    if ( p.valid && !( _cConnectedPads & ( 1 << player ) ) ) {
        Vec2 v( p.x, p.y );
        _lastSubStick[ player ] = v - position;
        return v;
    }

    Vec2 c;
    c.SetPolar( M_PI * ( _expansion[ player ]->classic.rjs.ang / 180.0f - 0.5f ), _expansion[ player ]->classic.rjs.mag );
    Vec2 v( PAD_SubStickX( player ), -PAD_SubStickY( player ) );

    if ( c.Length() >= 0.5f ) {
        _lastSubStick[ player ] = c;
    } else if ( v.Length() >= 20.0f )
        _lastSubStick[ player ] = v;
    _lastSubStick[ player ].Normalise();
    _lastSubStick[ player ] *= 32.0f;
    return position + _lastSubStick[ player ];
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

void LibWii::Render()
{
    GRRLIB_Render();
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
        return wPad & WPAD_BUTTON_A;
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
        return gPad & PAD_TRIGGER_R || gPad & PAD_BUTTON_X || gPad & PAD_BUTTON_Y;
    else if ( k == KEY_BOMB )
        return gPad & PAD_TRIGGER_L || gPad & PAD_BUTTON_A || gPad & PAD_BUTTON_B;
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

bool LibWii::HasNKey( u32 nPad, Key k )
{
    if ( k == KEY_BOMB )
        return nPad & WPAD_NUNCHUK_BUTTON_Z;

    return false;
}

bool LibWii::HasCKey( u32 cPad, Key k )
{
    if ( k == KEY_FIRE )
        return cPad & WPAD_CLASSIC_BUTTON_ZR || cPad & WPAD_CLASSIC_BUTTON_FULL_R || cPad & WPAD_CLASSIC_BUTTON_X || cPad & WPAD_CLASSIC_BUTTON_A;
    else if ( k == KEY_BOMB )
        return cPad & WPAD_CLASSIC_BUTTON_ZL || cPad & WPAD_CLASSIC_BUTTON_FULL_L || cPad & WPAD_CLASSIC_BUTTON_Y || cPad & WPAD_CLASSIC_BUTTON_B;
    else if ( k == KEY_ACCEPT )
        return cPad & WPAD_CLASSIC_BUTTON_A || cPad & WPAD_CLASSIC_BUTTON_B || cPad & WPAD_CLASSIC_BUTTON_ZR || cPad & WPAD_CLASSIC_BUTTON_FULL_R;
    else if ( k == KEY_CANCEL )
        return cPad & WPAD_CLASSIC_BUTTON_X || cPad & WPAD_CLASSIC_BUTTON_Y || cPad & WPAD_CLASSIC_BUTTON_ZL || cPad & WPAD_CLASSIC_BUTTON_FULL_L;
    else if ( k == KEY_MENU )
        return cPad & WPAD_CLASSIC_BUTTON_MINUS || cPad & WPAD_CLASSIC_BUTTON_PLUS || cPad & WPAD_CLASSIC_BUTTON_HOME;
    else if ( k == KEY_UP )
        return cPad & WPAD_CLASSIC_BUTTON_UP;
    else if ( k == KEY_DOWN )
        return cPad & WPAD_CLASSIC_BUTTON_DOWN;
    else if ( k == KEY_LEFT )
        return cPad & WPAD_CLASSIC_BUTTON_LEFT;
    else if ( k == KEY_RIGHT )
        return cPad & WPAD_CLASSIC_BUTTON_RIGHT;

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
#define USE_SOUND( sound, data ) _sounds.push_back( SoundResource( 0, NamedSound( sound , SoundBuffer( data , data##_len ) ) ) );

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

#endif
