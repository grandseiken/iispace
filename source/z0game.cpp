#include "z0game.h"
#include "lib.h"
#include "player.h"
#include "boss.h"
#include "enemy.h"
#include "overmind.h"
#include "stars.h"
#include <algorithm>
#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdintwin.h"
#endif
#include <cstring>

static int32_t state = 0;

void z_srand( int seed )
{
    state = seed;
}

int z_rand()
{
    int32_t const a = 1103515245;
    int32_t const c = 12345;
    state = a * state + c;
    return ( state >> 16 ) & 0x7fff;
}

#include "lookup.h"
#ifdef USE_FLOAT
int z_int( fixed x )
{
    return int( x );
}

float z_float( fixed x )
{
    return x;
}

fixed z_abs( fixed x )
{
    fixed r = x < 0 ? -x : x;
    DEBUG_STR( "z_abs called on " << x << " and returned " << r );
    return r;
}

fixed z_sin( fixed x )
{
#ifdef USE_SIN
    fixed r = ( x < 0 ? -1 : 1 ) * SIN_LOOKUP[ z_int( .5f + 65536.f * z_abs( x ) / M_2PI ) % 65536 ];
    DEBUG_STR( "z_sin called on " << x << " and returned " << r );
    return r;
#else
    return sin( x );
#endif
}

fixed z_cos( fixed x )
{
#ifdef USE_COS
    fixed r = COS_LOOKUP[ z_int( .5f + 65536.f * z_abs( x ) / M_2PI ) % 65536 ];
    DEBUG_STR( "z_cos called on " << x << " and returned " << r );
    return r;
#else
    return cos( x );
#endif
}

fixed z_atan2( fixed y, fixed x )
{
#ifdef USE_ATAN
    float f = .5f + 65536.f * x;
    fixed r;
    if ( f >= 65536.f )
        r = M_PI / M_FOUR;
    else
        r = ATAN_LOOKUP[ z_int( f ) % 65536 ];
    DEBUG_STR( "z_atan called on " << x << " and returned " << r );
    return r;
#else
    return atan2( y, x );
#endif
}
#endif

#ifdef USE_MPREAL
int z_int( fixed x )
{
    return int( x.toLong() );
}

float z_float( fixed x )
{
    return float( x.toDouble() );
}

fixed z_abs( fixed x )
{
    return abs( x );
}

fixed z_sqrt( fixed x )
{
    return sqrt( x );
}

fixed z_sin( fixed x )
{
#ifdef USE_SIN
    fixed r = ( x < 0 ? -1 : 1 ) * SIN_LOOKUP[ z_int( M_HALF + fixed( 65536 ) * z_abs( x ) / M_2PI ) % 65536 ];
    DEBUG_STR( "z_sin called on " << x << " and returned " << r );
    return r;
#else
    return sin( x );
#endif
}

fixed z_cos( fixed x )
{
#ifdef USE_COS
    fixed r = COS_LOOKUP[ z_int( M_HALF + fixed( 65536 ) * z_abs( x ) / M_2PI ) % 65536 ];
    DEBUG_STR( "z_cos called on " << x << " and returned " << r );
    return r;
#else
    return cos( x );
#endif
}

fixed z_atan2( fixed y, fixed x )
{
#ifdef USE_ATAN
    fixed f = M_HALF + fixed( 65536 ) * x;
    fixed r;
    if ( f >= fixed( 65536 ) )
        r = M_PI / M_FOUR;
    else
        r = ATAN_LOOKUP[ z_int( f ) % 65536 ];
    DEBUG_STR( "z_atan called on " << x << " and returned " << r );
    return r;
#else
    return atan2( y, x );
#endif
}
#endif

#ifdef USE_FIX32
int z_int( fixed x )
{
    return x.to_int();
}

float z_float( fixed x )
{
    return x.to_float();
}

fixed z_abs( fixed x )
{
    fixed r = x < 0 ? -x : x;
    return r;
}

fixed z_sqrt( fixed x )
{
    return x.sqrt();
}

fixed z_sin( fixed x )
{
    return x.sin();
}

fixed z_cos( fixed x )
{
    return x.cos();
}

fixed z_atan2( fixed y, fixed x )
{
    return y.atan2( x );
}
#endif

const int z0Game::STARTING_LIVES = 2;
const int z0Game::BOSSMODE_LIVES = 1;

z0Game::z0Game( Lib& lib, std::vector< std::string > args )
: Game( lib )
, _state( STATE_MENU )
, _players( 1 )
, _lives( 0 )
, _bossMode( false )
, _hardMode( false )
, _fastMode( false )
, _whatMode( false )
, _showHPBar( false )
, _fillHPBar( 0 )
, _selection( 0 )
, _specialSelection( 0 )
, _killTimer( 0 )
, _exitTimer( 0 )
, _enterChar( 0 )
, _enterTime( 0 )
, _scoreScreenTimer( 0 )
, _controllersConnected( 0 )
, _controllersDialog( false )
, _firstControllersDialog( false )
, _overmind( 0 )
, _bossesKilled( 0 )
, _hardModeBossesKilled( 0 )
{
    _overmind = new Overmind( *this );
    Lib::SaveData save = lib.LoadSaveData();
    _highScores = save._highScores;
    _bossesKilled = save._bossesKilled;
    _hardModeBossesKilled = save._hardModeBossesKilled;

    // Compliments have a max length of 24
    _compliments.push_back( " is a swell guy!" );
    _compliments.push_back( " went absolutely mental!" );
    _compliments.push_back( " is the bee's knees." );
    _compliments.push_back( " goes down in history!" );
    _compliments.push_back( " is old school!" );
    _compliments.push_back( ", oh, how cool you are!" );
    _compliments.push_back( " got the respect!" );
    _compliments.push_back( " had a cow!" );
    _compliments.push_back( " is a major badass." );
    _compliments.push_back( " is kickin' rad." );
    _compliments.push_back( " wins a coconut." );
    _compliments.push_back( " wins a kipper." );
    _compliments.push_back( " is probably cheating!" );
    _compliments.push_back( " is totally the best!" );
    _compliments.push_back( " ate your face!" );
    _compliments.push_back( " is feeling kinda funky." );
    _compliments.push_back( " is the cat's pyjamas." );
    _compliments.push_back( ", air guitar solo time!" );
    _compliments.push_back( ", give us a smile." );
    _compliments.push_back( " is a cheeky fellow!" );
    _compliments.push_back( " is a slippery customer!" );
    _compliments.push_back( "... that's is a puzzle!" );

    if ( !args.empty() ) {
        _replay = lib.PlayRecording( args[ 0 ] );
        if ( _replay._okay ) {
            _players = _replay._players;
            lib.SetPlayerCount( _players );
            Player::SetReplay( _replay );
            NewGame( _replay._canFaceSecretBoss, _replay._isBossMode, true, _replay._isHardMode, _replay._isFastMode, _replay._isWhatMode );
        }
        else {
            _exitTimer = 256;
        }
    }
}

z0Game::~z0Game()
{
#ifdef DEBUG
    debug.close();
#endif
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
        if ( _shipList[ i ]->IsEnemy() )
            _overmind->OnEnemyDestroy( _shipList[ i ] );
        delete _shipList[ i ];
    }
    for ( unsigned int i = 0; i < _particleList.size(); i++ )
        delete _particleList[ i ];

    delete _overmind;
}

void z0Game::Update()
{
    Lib& lib = GetLib();
    if ( _exitTimer ) {
        _exitTimer--;
        if ( !_exitTimer ) {
            lib.Exit( Lib::EXIT_TO_LOADER );
            _exitTimer = -1;
        }
        return;
    }

    for ( int i = 0; i < Lib::PLAYERS; i++ ) {
        if ( lib.IsKeyHeld( i, Lib::KEY_FIRE ) && lib.IsKeyHeld( i, Lib::KEY_BOMB ) && lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            lib.TakeScreenShot();
            break;
        }
    }
    lib.CaptureMouse( false );

    // Main menu
    //------------------------------
    if ( _state == STATE_MENU ) {

        if ( lib.IsKeyPressed( Lib::KEY_UP ) ) {
            _selection--;
            if ( _selection < 0 && !IsBossModeUnlocked() )
                _selection =  0;
            else if ( _selection < -1 && IsBossModeUnlocked() )
                _selection = -1;
            else
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
        }
        if ( lib.IsKeyPressed( Lib::KEY_DOWN ) ) {
            _selection++;
            if ( _selection > 2 )
                _selection = 2;
            else
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
        }

        if ( lib.IsKeyPressed( Lib::KEY_ACCEPT ) || lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            if ( _selection == 0 )
                NewGame( IsFastModeUnlocked() );
            else if ( _selection == 1 ) {
                _players++;
                if ( _players > Lib::PLAYERS )
                    _players = 1;
                lib.SetPlayerCount( _players );
            }
            else if ( _selection == 2 )
                _exitTimer = 2;
            else if ( _selection == -1 )
                NewGame( IsFastModeUnlocked(), _specialSelection == 0, false, _specialSelection == 1, _specialSelection == 2, _specialSelection == 3 );

            if ( _selection == 1 )
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            else
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }

        if ( _selection == 1 ) {
            int t = _players;
            if ( lib.IsKeyPressed( Lib::KEY_LEFT ) && _players > 1 )
                _players--;
            else if ( lib.IsKeyPressed( Lib::KEY_RIGHT ) && _players < Lib::PLAYERS )
                _players++;
            if ( t != _players )
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            GetLib().SetPlayerCount( _players );
        }

        if ( _selection == -1 ) {
            int t = _specialSelection;
            if ( lib.IsKeyPressed( Lib::KEY_LEFT ) && _specialSelection > 0 )
                _specialSelection--;
            else if ( lib.IsKeyPressed( Lib::KEY_RIGHT ) ) {
                if ( ( t == 0 && IsHardModeUnlocked() ) || ( t == 1 && IsFastModeUnlocked() ) || ( t == 2 && IsWhatModeUnlocked() ) )
                    _specialSelection++;
            }
            if ( t != _specialSelection )
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
        }

        if ( _selection >= 0 || _specialSelection == 0 )
            GetLib().SetColourCycle( 0 );
        else if ( _specialSelection == 1 )
            GetLib().SetColourCycle( 128 );
        else if ( _specialSelection == 2 )
            GetLib().SetColourCycle( 192 );
        else if ( _specialSelection == 3 )
            GetLib().SetColourCycle( ( GetLib().GetColourCycle() + 1 ) % 256 );

    }
    // Paused
    //------------------------------
    else if ( _state == STATE_PAUSE ) {

        int t = _selection;
        if ( lib.IsKeyPressed( Lib::KEY_UP ) && _selection > 0 )
            _selection--;
        if ( lib.IsKeyPressed( Lib::KEY_DOWN ) && _selection < 2 )
            _selection++;
        if ( t != _selection )
            GetLib().PlaySound( Lib::SOUND_MENU_CLICK );

        if ( lib.IsKeyPressed( Lib::KEY_ACCEPT ) || lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            if ( _selection == 0 ) {
                _state = STATE_GAME;
            }
            else if ( _selection == 1 ) {
                _state = STATE_HIGHSCORE;
                _enterChar = 0;
                _compliment = lib.RandInt( int( _compliments.size() ) );
                _killTimer = 0;
            }
            else if ( _selection == 2 )
                lib.SetVolume( std::min( 100, z_int( lib.LoadSettings()._volume ) + 1 ) );
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }
        if ( lib.IsKeyPressed( Lib::KEY_LEFT ) && _selection == 2 ) {
            int t = z_int( lib.LoadSettings()._volume );
            lib.SetVolume( std::max( 0, t - 1 ) );
            if ( z_int( lib.LoadSettings()._volume ) != t )
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
        }
        if ( lib.IsKeyPressed( Lib::KEY_RIGHT ) && _selection == 2 ) {
            int t = z_int( lib.LoadSettings()._volume );
            lib.SetVolume( std::min( 100, t + 1 ) );
            if ( z_int( lib.LoadSettings()._volume ) != t )
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
        }
        if ( lib.IsKeyPressed( Lib::KEY_CANCEL ) ) {
            _state = STATE_GAME;
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }

    }
    // In-game
    //------------------------------
    else if ( _state == STATE_GAME ) {

        Boss::_warnings.clear();
        lib.CaptureMouse( true );

        if ( IsHardMode() )
            GetLib().SetColourCycle( 128 );
        else if ( IsFastMode() )
            GetLib().SetColourCycle( 192 );
        else if ( IsWhatMode() )
            GetLib().SetColourCycle( ( GetLib().GetColourCycle() + 1 ) % 256 );
        else
            GetLib().SetColourCycle( 0 );

        if ( !Player::IsReplaying() && IsFastMode() )
            GetLib().SetFrameCount( 2 );
        else if ( !Player::IsReplaying() )
            GetLib().SetFrameCount( 1 );

        int controllers = 0;
        for ( int i = 0; i < CountPlayers(); i++ ) {
            controllers += lib.IsPadConnected( i );
        }
        if ( controllers < _controllersConnected && !_controllersDialog && !Player::IsReplaying() ) {
            _controllersDialog = true;
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }
        _controllersConnected = controllers;

        if ( _controllersDialog ) {
            if ( lib.IsKeyPressed( Lib::KEY_MENU ) || lib.IsKeyPressed( Lib::KEY_ACCEPT ) ) {
                _controllersDialog = false;
                _firstControllersDialog = false;
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
                for ( int i = 0; i < CountPlayers(); i++ ) {
                    GetLib().Rumble( i, 10 );
                }
            }
            return;
        }

        if ( lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            _state = STATE_PAUSE;
            _selection = 0;
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }

        if ( Player::IsReplaying() ) {
            if ( lib.IsKeyPressed( Lib::KEY_BOMB ) )
                lib.SetFrameCount( lib.GetFrameCount() * 2 );
            if ( lib.IsKeyPressed( Lib::KEY_FIRE ) && lib.GetFrameCount() > ( IsFastMode() ? 2 : 1 ) )
                lib.SetFrameCount( lib.GetFrameCount() / 2 );
        }

        Player::UpdateFireTimer();
        ChaserBoss::_hasCounted = false;
        std::stable_sort( _collisionList.begin(), _collisionList.end(), &SortShips );
        for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
            if ( !_shipList[ i ]->IsDestroyed() )
                _shipList[ i ]->Update();
        }
        for ( std::size_t i = 0; i < _particleList.size(); ++i ) {
            if ( !_particleList[ i ]->IsDestroyed() )
                _particleList[ i ]->Update();
        }
        for ( std::size_t i = 0; i < Boss::_fireworks.size(); ++i ) {
            if ( Boss::_fireworks[ i ].first <= 0 ) {
                Vec2 v = _shipList[ 0 ]->GetPosition();
                _shipList[ 0 ]->SetPosition( Boss::_fireworks[ i ].second.first );
                _shipList[ 0 ]->Explosion( 0xffffffff );
                _shipList[ 0 ]->Explosion( Boss::_fireworks[ i ].second.second, 16 );
                _shipList[ 0 ]->Explosion( 0xffffffff, 24 );
                _shipList[ 0 ]->Explosion( Boss::_fireworks[ i ].second.second, 32 );
                _shipList[ 0 ]->SetPosition( v );
                Boss::_fireworks.erase( Boss::_fireworks.begin() + i );
                --i;
            }
            else
                --Boss::_fireworks[ i ].first;
        }

        for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
            if ( _shipList[ i ]->IsDestroyed() ) {
                if ( _shipList[ i ]->IsEnemy() )
                    _overmind->OnEnemyDestroy( _shipList[ i ] );

                for ( unsigned int j = 0; j < _collisionList.size(); j++ )
                    if ( _collisionList[ j ] == _shipList[ i ] )
                        _collisionList.erase( _collisionList.begin() + j );

                delete _shipList[ i ];
                _shipList.erase( _shipList.begin() + i );
                i--;
            }
        }
        for ( std::size_t i = 0; i < _particleList.size(); ++i ) {
            if ( _particleList[ i ]->IsDestroyed() ) {
                delete _particleList[ i ];
                _particleList.erase( _particleList.begin() + i );
                i--;
            }
        }
        _overmind->Update();

        if ( ( ( int( Player::CountKilledPlayers() ) == _players && !GetLives() ) || ( IsBossMode() && _overmind->GetKilledBosses() >= 6 ) ) && !_killTimer ) {
            _killTimer = 100;
        }
        if ( _killTimer ) {
            _killTimer--;
            if ( !_killTimer ) {
                _state = STATE_HIGHSCORE;
                _scoreScreenTimer = 0;
                _enterChar = 0;
                _compliment = lib.RandInt( int( _compliments.size() ) );
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
            }
        }

    }
    // Entering high-score
    //------------------------------
    else if ( _state == STATE_HIGHSCORE )
    {
        ++_scoreScreenTimer;
        GetLib().OnScore( _replay._seed, _players, IsBossMode(), IsBossMode() ? ( _overmind->GetKilledBosses() >= 6 && _overmind->GetElapsedTime() != 0 ? std::max( _overmind->GetElapsedTime() - 600l * GetLives(), 1l ) : 0l ) : GetTotalScore(), IsHardMode(), IsFastMode(), IsWhatMode() );
        if ( IsHighScore() ) {
            _enterTime++;
            std::string chars = ALLOWED_CHARS;
            if ( lib.IsKeyPressed( Lib::KEY_ACCEPT ) && _enterName.length() < Lib::MAX_NAME_LENGTH ) {
                _enterName += chars.substr( _enterChar, 1 );
                _enterTime = 0;
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyPressed( Lib::KEY_CANCEL ) && _enterName.length() > 0 ) {
                _enterName = _enterName.substr( 0, _enterName.length() - 1 );
                _enterTime = 0;
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyPressed( Lib::KEY_RIGHT ) ) {
                _enterR = 0;
                _enterChar++;
                if ( _enterChar >= int( chars.length() ) )
                    _enterChar -= int( chars.length() );
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyHeld( Lib::KEY_RIGHT ) ) {
                _enterR++;
                _enterTime = 16;
                if ( _enterR % 5 == 0 && _enterR > 5 ) {
                    _enterChar++;
                    if ( _enterChar >= int( chars.length() ) )
                        _enterChar -= int( chars.length() );
                    GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
                }
            }
            if ( lib.IsKeyPressed( Lib::KEY_LEFT ) ) {
                _enterR = 0;
                _enterChar--;
                if ( _enterChar < 0 )
                    _enterChar += int( chars.length() );
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyHeld( Lib::KEY_LEFT ) ) {
                _enterR++;
                _enterTime = 16;
                if ( _enterR % 5 == 0 && _enterR > 5 ) {
                    _enterChar--;
                    if ( _enterChar < 0 )
                        _enterChar += int( chars.length() );
                    GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
                }
            }
            if ( lib.IsKeyPressed( Lib::KEY_MENU ) ) {
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
                if ( !IsBossMode() ) {
                    int n = IsWhatMode() ? 3 * Lib::PLAYERS + _players : IsFastMode() ? 2 * Lib::PLAYERS + _players : IsHardMode() ? Lib::PLAYERS + _players : _players - 1;
                    Lib::HighScoreList& list = _highScores[ n ];
                    list.push_back( Lib::HighScore( _enterName, GetTotalScore() ) );
                    std::stable_sort( list.begin(), list.end(), &Lib::ScoreSort );
                    list.erase( list.begin() + ( list.size() - 1 ) );
                    GetLib().EndRecording( _enterName, GetTotalScore(), _players, false, IsHardMode(), IsFastMode(), IsWhatMode() );
                    EndGame();
                } else {
                    long score = _overmind->GetElapsedTime();
                    score -= 600l * GetLives();
                    if ( score <= 0 )
                        score = 1;
                    _highScores[ Lib::PLAYERS ][ _players - 1 ].first = _enterName;
                    _highScores[ Lib::PLAYERS ][ _players - 1 ].second = score;
                    GetLib().EndRecording( _enterName, score, _players, true, IsHardMode(), IsFastMode(), IsWhatMode() );
                    EndGame();
                }
            }
        }
        else if ( lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            GetLib().EndRecording( "untitled", IsBossMode() ? ( _overmind->GetKilledBosses() >= 6 && _overmind->GetElapsedTime() != 0 ? std::max( _overmind->GetElapsedTime() - 600l * GetLives(), 1l ) : 0l ) : GetTotalScore(), _players, IsBossMode(), IsHardMode(), IsFastMode(), IsWhatMode() );
            EndGame();
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }
    }
}

void z0Game::Render() const
{
    const Lib& lib = GetLib();

    if ( _exitTimer > 0 && !_replay._okay && _replay._error.size() > 0 ) {
        int y = 2;
        std::string s = _replay._error;
        std::size_t i;
        while ( ( i = s.find_first_of( "\n" ) ) != std::string::npos ) {
            std::string t = s.substr( 0, i );
            s = s.substr( i + 1 );
            lib.RenderText( Vec2f( 2, float( y ) ), t, PANEL_TEXT );
            ++y;
        }
        lib.RenderText( Vec2f( 2, float( y ) ), s, PANEL_TEXT );
        return;
    }

    _showHPBar = false;
    _fillHPBar = 0;
    if ( !_firstControllersDialog ) {
        Star::Render();
        for ( std::size_t i = 0; i < _particleList.size(); ++i )
            _particleList[ i ]->Render();
        for ( std::size_t i = CountPlayers(); i < _shipList.size(); ++i )
            _shipList[ i ]->Render();
        for ( std::size_t i = 0; i < unsigned( CountPlayers() ) && i < _shipList.size(); ++i )
            _shipList[ i ]->Render();
    }

    // In-game
    //------------------------------
    if ( _state == STATE_GAME ) {

        if ( _controllersDialog ) {

            RenderPanel( Vec2f( 3.f, 3.f ), Vec2f( 32.f, 8.f + 2 * CountPlayers() ) );

            lib.RenderText( Vec2f( 4.f, 4.f ), "CONTROLLERS FOUND", PANEL_TEXT );

            for ( int i = 0; i < CountPlayers(); i++ ) {
                std::stringstream ss;
                ss << "PLAYER " << ( i + 1 ) << ": ";
                lib.RenderText( Vec2f( 4.f, 8.f + 2 * i ), ss.str(), PANEL_TEXT );

                std::stringstream ss2;
                int pads = lib.IsPadConnected( i );
                if ( Player::IsReplaying() ) {
                    ss2 << "REPLAY";
                    pads = 1;
                }
                else {
                    if ( pads & Lib::PAD_GAMECUBE ) {
                        ss2 << "GAMECUBE";
                        if ( pads & Lib::PAD_WIIMOTE || pads & Lib::PAD_CLASSIC )
                            ss2 << ", ";
                    }
                    if ( pads & Lib::PAD_CLASSIC )
                        ss2 << "CLASSIC";
                    else if ( pads & Lib::PAD_WIIMOTE )
                        ss2 << "WIIMOTE";
                    if ( !pads )
                        ss2 << "NONE";
                    if ( pads & Lib::PAD_GAMEPAD ) {
                        ss2 << "GAMEPAD";
                        if ( pads & Lib::PAD_KEYMOUSE )
                            ss2 << ", KB/MOUSE";
                    }
                    else if ( pads & Lib::PAD_KEYMOUSE )
                        ss2 << "MOUSE & KEYBOARD";
                }

                lib.RenderText( Vec2f( 14.f, 8.f + 2 * i ), ss2.str(), pads ? Player::GetPlayerColour( i ) : PANEL_TEXT );
            }
            return;
        }

        std::stringstream ss;
        ss << _lives << " live(s)";
        if ( !IsBossMode() && _overmind->GetTimer() >= 0 ) {
            int t = int( 0.5f + _overmind->GetTimer() / 60 );
            ss << " " << ( t < 10 ? "0" : "" ) << t;
        }
        lib.RenderText( Vec2f( Lib::WIDTH / ( 2.f * Lib::TEXT_WIDTH ) - ss.str().length() / 2, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f - lib.LoadSettings()._hudCorrection ),
                        ss.str(), PANEL_TRAN );
        ShipList list = GetShipsInRadius( Vec2(), Lib::WIDTH * 100 );
        for ( std::size_t i = 0; i < list.size() + Boss::_warnings.size(); ++i ) {
            if ( i < list.size() && !list[ i ]->IsEnemy() )
                continue;
            Vec2f v = Vec2f( i < list.size() ? list[ i ]->GetPosition() : Boss::_warnings[ i - list.size() ] );

            if ( v._x < -4 ) {
                int a = int( .5f + float( 0x1 ) + float( 0x9 ) * std::max( v._x + Lib::WIDTH, 0.f ) / Lib::WIDTH );
                a |= a << 4;
                a = ( a << 8 ) | ( a << 16 ) | ( a << 24 ) | 0x66;
                lib.RenderLine( Vec2f( 0.f, v._y ),     Vec2f( 6, v._y - 3 ), a );
                lib.RenderLine( Vec2f( 6.f, v._y - 3 ), Vec2f( 6, v._y + 3 ), a );
                lib.RenderLine( Vec2f( 6.f, v._y + 3 ), Vec2f( 0, v._y ), a );
            }
            if ( v._x >= Lib::WIDTH + 4 ) {
                int a = int( .5f + float( 0x1 ) + float( 0x9 ) * std::max( 2 * Lib::WIDTH - v._x, 0.f ) / Lib::WIDTH );
                a |= a << 4;
                a = ( a << 8 ) | ( a << 16 ) | ( a << 24 ) | 0x66;
                lib.RenderLine( Vec2f( float( Lib::WIDTH ), v._y ), Vec2f( Lib::WIDTH - 6.f, v._y - 3 ), a );
                lib.RenderLine( Vec2f( Lib::WIDTH - 6, v._y - 3 ),  Vec2f( Lib::WIDTH - 6.f, v._y + 3 ), a );
                lib.RenderLine( Vec2f( Lib::WIDTH - 6, v._y + 3 ),  Vec2f( float( Lib::WIDTH ), v._y ), a );
            }
            if ( v._y < -4 ) {
                int a = int( .5f + float( 0x1 ) + float( 0x9 ) * std::max( v._y + Lib::HEIGHT, 0.f ) / Lib::HEIGHT );
                a |= a << 4;
                a = ( a << 8 ) | ( a << 16 ) | ( a << 24 ) | 0x66;
                lib.RenderLine( Vec2f( v._x, 0.f ),     Vec2f( v._x - 3, 6.f ), a );
                lib.RenderLine( Vec2f( v._x - 3, 6.f ), Vec2f( v._x + 3, 6.f ), a );
                lib.RenderLine( Vec2f( v._x + 3, 6.f ), Vec2f( v._x, 0.f ), a );
            }
            if ( v._y >= Lib::HEIGHT + 4 ) {
                int a = int( .5f + float( 0x1 ) + float( 0x9 ) * std::max( 2 * Lib::HEIGHT - v._y, 0.f ) / Lib::HEIGHT );
                a |= a << 4;
                a = ( a << 8 ) | ( a << 16 ) | ( a << 24 ) | 0x66;
                lib.RenderLine( Vec2f( v._x, float( Lib::HEIGHT ) ),  Vec2f( v._x - 3, Lib::HEIGHT - 6.f ), a );
                lib.RenderLine( Vec2f( v._x - 3, Lib::HEIGHT - 6.f ), Vec2f( v._x + 3, Lib::HEIGHT - 6.f ), a );
                lib.RenderLine( Vec2f( v._x + 3, Lib::HEIGHT - 6.f ), Vec2f( v._x, float( Lib::HEIGHT ) ), a );
            }
        }
        if ( IsBossMode() ) {
            std::stringstream sst;
            sst << ConvertToTime( _overmind->GetElapsedTime() );
            lib.RenderText( Vec2f( Lib::WIDTH / ( 2 * Lib::TEXT_WIDTH ) - sst.str().length() - 1.f, 1.f + lib.LoadSettings()._hudCorrection ),
                            sst.str(), PANEL_TRAN );
        }
        if ( _showHPBar ) {
            int x = IsBossMode() ? 48 : 0;
            lib.RenderRect( Vec2f( x + Lib::WIDTH / 2 - 48.f, 16.f * ( 1 + lib.LoadSettings()._hudCorrection ) ),
                            Vec2f( x + Lib::WIDTH / 2 + 48.f, 16.f * ( 2 + lib.LoadSettings()._hudCorrection ) ), PANEL_TRAN, 2 );
            lib.RenderRect( Vec2f( x + Lib::WIDTH / 2 - 44.f, 16.f * ( 1 + lib.LoadSettings()._hudCorrection ) + 4 ),
                            Vec2f( x + Lib::WIDTH / 2 - 44.f + 88.f * _fillHPBar, 16.f * ( 2 + lib.LoadSettings()._hudCorrection ) - 4 ), PANEL_TRAN );
        }
        if ( Player::IsReplaying() ) {
            int x = IsFastMode() ? lib.GetFrameCount() / 2 : lib.GetFrameCount();
            std::stringstream ss;
            ss << x << "X " << int( 100 * float( Player::ReplayFrame() ) / _replay._playerFrames.size() ) << "%";
            lib.RenderText( Vec2f( Lib::WIDTH / ( 2.f * Lib::TEXT_WIDTH ) - ss.str().length() / 2, Lib::HEIGHT / Lib::TEXT_HEIGHT - 3.f - lib.LoadSettings()._hudCorrection ), ss.str(), PANEL_TRAN );
        }

    }
    // Main menu screen
    //------------------------------
    else if ( _state == STATE_MENU ) {

        // Main menu
        //------------------------------
        RenderPanel( Vec2f( 3.f, 3.f ), Vec2f( 19.f, 14.f ) );

        lib.RenderText( Vec2f( 37.f - 16, 3.f ),  "coded by: SEiKEN", PANEL_TEXT );
        lib.RenderText( Vec2f( 37.f - 16, 4.f ),  "stu@seiken.co.uk", PANEL_TEXT );
        lib.RenderText( Vec2f( 37.f - 9, 6.f ),          "-testers-", PANEL_TEXT );
        lib.RenderText( Vec2f( 37.f - 9, 7.f ),          "MATT BELL", PANEL_TEXT );
        lib.RenderText( Vec2f( 37.f - 9, 8.f ),          "RUFUZZZZZ", PANEL_TEXT );
        lib.RenderText( Vec2f( 37.f - 9, 9.f ),          "SHADOW1W2", PANEL_TEXT );

        std::string b = "BOSSES:  ";
        int bb = IsHardModeUnlocked() ? _hardModeBossesKilled : _bossesKilled;
        if ( bb & BOSS_1A ) b += "X"; else b += "-";
        if ( bb & BOSS_1B ) b += "X"; else b += "-";
        if ( bb & BOSS_1C ) b += "X"; else b += "-";
        if ( bb & BOSS_3A ) b += "X"; else b += " ";
        if ( bb & BOSS_2A ) b += "X"; else b += "-";
        if ( bb & BOSS_2B ) b += "X"; else b += "-";
        if ( bb & BOSS_2C ) b += "X"; else b += "-";
        lib.RenderText( Vec2f( 37.f - 16, 13.f ), b, PANEL_TEXT );

        lib.RenderText( Vec2f( 4.f, 4.f ),  "WiiSPACE",   PANEL_TEXT );
        lib.RenderText( Vec2f( 6.f, 8.f ),  "START GAME", PANEL_TEXT );
        lib.RenderText( Vec2f( 6.f, 10.f ), "PLAYERS",    PANEL_TEXT );
        lib.RenderText( Vec2f( 6.f, 12.f ), "EXiT",       PANEL_TEXT );

        if ( _specialSelection == 0 && IsBossModeUnlocked() ) {
            lib.RenderText( Vec2f( 6.f, 6.f ), "BOSS MODE", PANEL_TEXT );
            if ( IsHardModeUnlocked() && _selection == -1 )
                lib.RenderText( Vec2f( 6.f, 6.f ), "         >", PANEL_TRAN );
        }
        if ( _specialSelection == 1 && IsHardModeUnlocked() ) {
            lib.RenderText( Vec2f( 6.f, 6.f ), "HARD MODE", PANEL_TEXT );
            if ( _selection == -1 )
                lib.RenderText( Vec2f( 5.f, 6.f ), "<", PANEL_TRAN );
            if ( IsFastModeUnlocked() && _selection == -1 )
                lib.RenderText( Vec2f( 6.f, 6.f ), "         >", PANEL_TRAN );
        }
        if ( _specialSelection == 2 && IsFastModeUnlocked() ) {
            lib.RenderText( Vec2f( 6.f, 6.f ), "FAST MODE", PANEL_TEXT );
            if ( _selection == -1 )
                lib.RenderText( Vec2f( 5.f, 6.f ), "<", PANEL_TRAN );
            if ( IsWhatModeUnlocked() && _selection == -1 )
                lib.RenderText( Vec2f( 6.f, 6.f ), "         >", PANEL_TRAN );
        }
        if ( _specialSelection == 3 && IsWhatModeUnlocked() ) {
            lib.RenderText( Vec2f( 6.f, 6.f ), "W-HAT MODE", PANEL_TEXT );
            if ( _selection == -1 )
                lib.RenderText( Vec2f( 5.f, 6.f ), "<", PANEL_TRAN );
        }

        Vec2f low( float( 4 * Lib::TEXT_WIDTH + 4 ), float( ( 8 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 ) );
        Vec2f  hi( float( 5 * Lib::TEXT_WIDTH - 4 ), float( ( 9 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 ) );
        lib.RenderRect( low, hi, PANEL_TEXT, 1 );

        if ( _players > 1 && _selection == 1 )
            lib.RenderText( Vec2f( 5.f, 10.f ), "<", PANEL_TRAN );
        if ( _players < 4 && _selection == 1 )
            lib.RenderText( Vec2f( 14.f + _players, 10.f ), ">", PANEL_TRAN );
        for ( int i = 0; i < _players; i++ ) {
            std::stringstream ss;
            ss << ( i + 1 );
            lib.RenderText( Vec2f( 14.f + i, 10.f ), ss.str(), Player::GetPlayerColour( i ) );
        }

        // High score table
        //------------------------------
        RenderPanel( Vec2f( 3.f, 15.f ), Vec2f( 37.f, 27.f ) );

        std::stringstream ss;
        ss << _players;
        std::string s        = "HiGH SCORES    ";
        s += _selection == -1 ? ( 
                 _specialSelection == 0 ? "BOSS MODE" :
                 _specialSelection == 1 ? "HARD MODE (" + ss.str() + "P)" :
                 _specialSelection == 2 ? "FAST MODE (" + ss.str() + "P)" :
                                          "W-HAT MODE (" + ss.str() + "P)"
             ) :
             _players   ==  1 ?    "ONE PLAYER" :
             _players   ==  2 ?   "TWO PLAYERS" :
             _players   ==  3 ? "THREE PLAYERS" :
             _players   ==  4 ?  "FOUR PLAYERS" : "";
        lib.RenderText( Vec2f( 4.f, 16.f ), s, PANEL_TEXT );

        if ( _selection == -1 && _specialSelection == 0 ) {
            lib.RenderText( Vec2f( 4.f, 18.f ), "ONE PLAYER", PANEL_TEXT );
            lib.RenderText( Vec2f( 4.f, 20.f ), "TWO PLAYERS", PANEL_TEXT );
            lib.RenderText( Vec2f( 4.f, 22.f ), "THREE PLAYERS", PANEL_TEXT );
            lib.RenderText( Vec2f( 4.f, 24.f ), "FOUR PLAYERS", PANEL_TEXT );

            for ( int i = 0; i < Lib::PLAYERS; i++ ) {
                std::string score = ConvertToTime( _highScores[ Lib::PLAYERS ][ i ].second );
                if ( score.length() > Lib::MAX_SCORE_LENGTH )
                    score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
                std::string name = _highScores[ Lib::PLAYERS ][ i ].first;
                if ( name.length() > Lib::MAX_NAME_LENGTH )
                    name = name.substr( 0, Lib::MAX_NAME_LENGTH );

                lib.RenderText( Vec2f( 19.f, 18.f + i * 2 ), score, PANEL_TEXT );
                lib.RenderText( Vec2f( 19.f, 19.f + i * 2 ), name, PANEL_TEXT );
            }
        }
        else {
            for ( unsigned int i = 0; i < Lib::NUM_HIGH_SCORES; i++ ) {
                std::stringstream ssi;
                ssi << ( i + 1 ) << ".";
                lib.RenderText( Vec2f( 4.f, 18.f + i ), ssi.str(), PANEL_TEXT );

                int n = _selection != -1 ? _players - 1 :
                        _specialSelection * Lib::PLAYERS + _players;

                if ( _highScores[ n ][ i ].second <= 0 )
                    continue;

                std::stringstream ss;
                ss << _highScores[ n ][ i ].second;
                std::string score = ss.str();
                if ( score.length() > Lib::MAX_SCORE_LENGTH )
                    score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
                std::string name = _highScores[ n ][ i ].first;
                if ( name.length() > Lib::MAX_NAME_LENGTH )
                    name = name.substr( 0, Lib::MAX_NAME_LENGTH );

                lib.RenderText( Vec2f( 7.f, 18.f + i ),  score, PANEL_TEXT );
                lib.RenderText( Vec2f( 19.f, 18.f + i ), name, PANEL_TEXT );
            }
        }
    }
    // Pause menu
    //------------------------------
    else if ( _state == STATE_PAUSE ) {

        RenderPanel( Vec2f( 3.f, 3.f ), Vec2f( 15.f, 14.f ) );

        lib.RenderText( Vec2f(  4.f, 4.f ),  "PAUSED",   PANEL_TEXT );
        lib.RenderText( Vec2f(  6.f, 8.f ),  "CONTINUE", PANEL_TEXT );
        lib.RenderText( Vec2f(  6.f, 10.f ), "END GAME", PANEL_TEXT );
        lib.RenderText( Vec2f(  6.f, 12.f ), "VOL.",     PANEL_TEXT );
        std::stringstream vol;
        int v = z_int( lib.LoadSettings()._volume );
        vol << " " << ( v < 10 ? " " : "" ) << v;
        lib.RenderText( Vec2f( 10.f, 12.f ), vol.str(), PANEL_TEXT );
        if ( _selection == 2 && z_int( lib.LoadSettings()._volume ) > 0 )
            lib.RenderText( Vec2f( 5.f, 12.f ), "<", PANEL_TRAN );
        if ( _selection == 2 && z_int( lib.LoadSettings()._volume ) < 100 )
            lib.RenderText( Vec2f( 13.f, 12.f ), ">", PANEL_TRAN );

        Vec2f low( float( 4 * Lib::TEXT_WIDTH + 4 ), float( ( 8 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 ) );
        Vec2f  hi( float( 5 * Lib::TEXT_WIDTH - 4 ), float( ( 9 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 ) );
        lib.RenderRect( low, hi, PANEL_TEXT, 1 );
    }
    // Score screen
    //------------------------------
    else if ( _state == STATE_HIGHSCORE ) {

        GetLib().SetFrameCount( 1 );

        // Name enter
        //------------------------------
        if ( IsHighScore() ) {
            std::string chars = ALLOWED_CHARS;

            RenderPanel( Vec2f( 3.f, 20.f ), Vec2f( 28.f, 27.f ) );
            lib.RenderText( Vec2f( 4.f, 21.f ), "It's a high score!", PANEL_TEXT );
            lib.RenderText( Vec2f( 4.f, 23.f ), _players == 1 ? "Enter name:" : "Enter team name:", PANEL_TEXT );
            lib.RenderText( Vec2f( 6.f, 25.f ), _enterName, PANEL_TEXT );
            if ( ( _enterTime / 16 ) % 2 && _enterName.length() < Lib::MAX_NAME_LENGTH ) {
                lib.RenderText( Vec2f( 6.f + _enterName.length(), 25.f ), chars.substr( _enterChar, 1 ), 0xbbbbbbff );
            }
            Vec2f low( float( 4 * Lib::TEXT_WIDTH + 4 ), float( ( 25 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 ) );
            Vec2f  hi( float( 5 * Lib::TEXT_WIDTH - 4 ), float( ( 26 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 ) );
            lib.RenderRect( low, hi, PANEL_TEXT, 1 );
        }

        // Boss mode score listing
        //------------------------------
        if ( IsBossMode() ) {
            int extraLives = GetLives();
            bool b = extraLives > 0 && _overmind->GetKilledBosses() >= 6;

            long score = _overmind->GetElapsedTime();
            if ( b )
                score -= 10 * extraLives;
            if ( score <= 0 )
                score = 1;

            RenderPanel( Vec2f( 3.f, 3.f ), Vec2f( 37.f, b ? 10.f : 8.f ) );
            if ( b ) {
                std::stringstream ss;
                ss << ( extraLives * 10 ) << "-second extra-life bonus!";
                lib.RenderText( Vec2f( 4.f, 4.f ), ss.str(), PANEL_TEXT );
            }

            lib.RenderText( Vec2f( 4.f, b ? 6.f : 4.f ), "TIME ELAPSED: " + ConvertToTime( score ), PANEL_TEXT );
            std::stringstream ss;
            ss << "BOSS DESTROY: " << _overmind->GetKilledBosses();
            lib.RenderText( Vec2f( 4.f, b ? 8.f : 6.f ), ss.str(), PANEL_TEXT );
            return;
        }

        // Score listing
        //------------------------------
        RenderPanel( Vec2f( 3.f, 3.f ), Vec2f( 37.f, 8.f + 2 * _players + ( _players > 1 ? 2 : 0 ) ) );

        std::stringstream ss;
        ss << GetTotalScore();
        std::string score = ss.str();
        if ( score.length() > Lib::MAX_SCORE_LENGTH )
            score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
        lib.RenderText( Vec2f( 4.f, 4.f ), "TOTAL SCORE: " + score, PANEL_TEXT );

        for ( int i = 0; i < _players; i++ ) {
            std::stringstream sss;
            if ( _scoreScreenTimer % 600 < 300 )
                sss << GetPlayerScore( i );
            else
                sss << GetPlayerDeaths( i ) << " death" << ( GetPlayerDeaths( i ) != 1 ? "s" : "" );
            score = sss.str();
            if ( score.length() > Lib::MAX_SCORE_LENGTH )
                score = score.substr( 0, Lib::MAX_SCORE_LENGTH );

            std::stringstream ssp;
            ssp << "PLAYER " << ( i + 1 ) << ":";
            lib.RenderText( Vec2f( 4.f, 8.f + 2 * i ),  ssp.str(), PANEL_TEXT );
            lib.RenderText( Vec2f( 14.f, 8.f + 2 * i ), score,     Player::GetPlayerColour( i ) );
        }

        // Top-ranked player
        //------------------------------
        if ( _players > 1 ) {
            long max = -1;
            int best = -1;
            for ( int i = 0; i < _players; i++ ) {
                if ( GetPlayerScore( i ) > max ) {
                    max  = GetPlayerScore( i );
                    best = i;
                }
            }

            if ( GetTotalScore() > 0 ) {
                std::stringstream s;
                s << "PLAYER " << ( best + 1 );
                lib.RenderText( Vec2f( 4.f, 8.f + 2 * _players ), s.str(), Player::GetPlayerColour( best ) );

                std::string compliment = _compliments[ _compliment ];
                lib.RenderText( Vec2f( 12.f, 8.f + 2 * _players ), compliment, PANEL_TEXT );
            }
            else {
                lib.RenderText( Vec2f( 4.f, 8.f + 2 * _players ), "Oh dear!", PANEL_TEXT );
            }
        }

    }
}

// Game control
//------------------------------
void z0Game::NewGame( bool canFaceSecretBoss, bool bossMode, bool replay, bool hardMode, bool fastMode, bool whatMode )
{
    if ( !replay ) {
        Player::DisableReplay();
        GetLib().StartRecording( _players, canFaceSecretBoss, bossMode, hardMode, fastMode, whatMode );
    }
    _controllersDialog = true;
    _firstControllersDialog = true;
    _controllersConnected = 0;
    _bossMode = bossMode;
    _hardMode = hardMode;
    _fastMode = fastMode;
    _whatMode = whatMode;
    _overmind->Reset( canFaceSecretBoss );
    _lives = bossMode ? _players * BOSSMODE_LIVES : STARTING_LIVES;
    GetLib().SetFrameCount( IsFastMode() ? 2 : 1 );

    Star::Clear();
    for ( int i = 0; i < _players; i++ ) {
        Vec2 v( fixed( ( 1 + i ) * Lib::WIDTH / ( 1 + _players ) ), fixed( Lib::HEIGHT / 2 ) );
        Player* p = new Player( v, i );
        AddShip( p );
        _playerList.push_back( p );
    }
    _state = STATE_GAME;
}

void z0Game::EndGame()
{
    for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
        if ( _shipList[ i ]->IsEnemy() )
            _overmind->OnEnemyDestroy( _shipList[ i ] );
        delete _shipList[ i ];
    }
    for ( std::size_t i = 0; i < _particleList.size(); ++i )
        delete _particleList[ i ];

    Star::Clear();
    _particleList.clear();
    _shipList.clear();
    _playerList.clear();
    _collisionList.clear();
    Boss::_fireworks.clear();
    Boss::_warnings.clear();

    Lib::SaveData save2;
    save2._highScores = _highScores;
    save2._bossesKilled = _bossesKilled;
    save2._hardModeBossesKilled = _hardModeBossesKilled;
    GetLib().SaveSaveData( save2 );
    GetLib().EndRecording( "untitled", 0, _players, IsBossMode(), IsHardMode(), IsFastMode(), IsWhatMode() );
    GetLib().SetFrameCount( 1 );
    _state = STATE_MENU;
    _selection = ( ( IsBossModeUnlocked() && IsBossMode() ) || ( IsHardMode() && IsHardModeUnlocked() ) || ( IsFastMode() && IsFastModeUnlocked() ) || ( IsWhatMode() && IsWhatModeUnlocked() ) ) ? -1 : 0;
    _specialSelection = ( IsBossModeUnlocked() && IsBossMode() ) ? 0 : ( IsHardMode() && IsHardModeUnlocked() ) ? 1 : ( IsFastMode() && IsFastModeUnlocked() ) ? 2 : ( IsWhatMode() && IsWhatModeUnlocked() ) ? 3 : 0;
}

void z0Game::AddShip( Ship* ship )
{
    ship->SetGame( *this );
    if ( ship->IsEnemy() )
        _overmind->OnEnemyCreate( ship );
    _shipList.push_back( ship );

    if ( ship->GetBoundingWidth() > 1 )
        _collisionList.push_back( ship );
}

void z0Game::AddParticle( Particle* particle )
{
    particle->SetGame( *this );
    _particleList.push_back( particle );
}

// Ship info
//------------------------------
int z0Game::GetNonWallCount() const
{
    return _overmind->CountNonWallEnemies();
}

z0Game::ShipList z0Game::GetCollisionList( const Vec2& point, int category ) const
{
    ShipList r;
    fixed x = point._x;
    fixed y = point._y;

    for ( unsigned int i = 0; i < _collisionList.size(); i++ ) {
        fixed sx = _collisionList[ i ]->GetPosition()._x;
        fixed sy = _collisionList[ i ]->GetPosition()._y;
        fixed w  = _collisionList[ i ]->GetBoundingWidth();

        if ( sx - w > x )
            break;
        if ( sx + w < x || sy + w < y || sy - w > y )
            continue;

        if ( _collisionList[ i ]->CheckPoint( point, category ) )
            r.push_back( _collisionList[ i ] );
    }
    return r;
}

z0Game::ShipList z0Game::GetShipsInRadius( const Vec2& point, fixed radius ) const
{
    ShipList r;
    for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
        if ( ( _shipList[ i ]->GetPosition() - point ).Length() <= radius )
            r.push_back( _shipList[ i ] );
    }
    return r;
}

const z0Game::ShipList& z0Game::GetShips() const
{
    return _shipList;
}

bool z0Game::AnyCollisionList( const Vec2& point, int category ) const
{
    fixed x = point._x;
    fixed y = point._y;

    for ( unsigned int i = 0; i < _collisionList.size(); i++ ) {
        fixed sx = _collisionList[ i ]->GetPosition()._x;
        fixed sy = _collisionList[ i ]->GetPosition()._y;
        fixed w  = _collisionList[ i ]->GetBoundingWidth();

        if ( sx - w > x )
            break;
        if ( sx + w < x || sy + w < y || sy - w > y )
            continue;

        if ( _collisionList[ i ]->CheckPoint( point, category ) )
            return true;
    }
    return false;
}

Player* z0Game::GetNearestPlayer( const Vec2& point ) const
{
    Ship* r = 0;
    Ship* deadR = 0;
    fixed d = Lib::WIDTH * Lib::HEIGHT;
    fixed deadD = Lib::WIDTH * Lib::HEIGHT;

    for ( unsigned int i = 0; i < _playerList.size(); i++ ) {
        if ( !( ( Player* )_playerList[ i ] )->IsKilled() && ( _playerList[ i ]->GetPosition() - point ).Length() < d ) {
            d = ( _playerList[ i ]->GetPosition() - point ).Length();
            r = _playerList[ i ];
        }
        if ( ( _playerList[ i ]->GetPosition() - point ).Length() < deadD ) {
            deadD = ( _playerList[ i ]->GetPosition() - point ).Length();
            deadR = _playerList[ i ];
        }
    }
    return ( Player* )( r != 0 ? r : deadR );
}

void z0Game::SetBossKilled( BossList boss )
{
    if ( Player::IsReplaying() )
        return;
    if ( IsHardMode() || IsFastMode() || IsWhatMode() || boss == BOSS_3A )
        _hardModeBossesKilled |= boss;
    else
        _bossesKilled |= boss;
}

bool z0Game::SortShips( Ship* const& a, Ship* const& b )
{
    return a->GetPosition()._x - a->GetBoundingWidth() < b->GetPosition()._x - b->GetBoundingWidth();
}

// UI layout
//------------------------------
void z0Game::RenderPanel( const Vec2f& low, const Vec2f& hi ) const
{
    Vec2f tlow( low._x * Lib::TEXT_WIDTH, low._y * Lib::TEXT_HEIGHT );
    Vec2f  thi(  hi._x * Lib::TEXT_WIDTH,  hi._y * Lib::TEXT_HEIGHT );
    GetLib().RenderRect( tlow, thi, PANEL_BACK );
    GetLib().RenderRect( tlow, thi, PANEL_TEXT, 4 );
}

std::string z0Game::ConvertToTime( long score ) const
{
    if ( score == 0 )
        return "--:--";
    int mins = 0;
    while ( score >= 60 * 60 && mins < 99 ) {
        score -= 60 * 60;
        mins++;
    }
    int secs = score / 60;

    std::stringstream r;
    if ( mins < 10 )
        r << "0";
    r << mins << ":";
    if ( secs < 10 )
        r << "0";
    r << secs;
    return r.str();
}

// Score calculation
//------------------------------
long z0Game::GetPlayerScore( int playerNumber ) const
{
    for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
        if ( _shipList[ i ]->IsPlayer() ) {
            Player* p = ( Player* )_shipList[ i ];
            if ( p->GetPlayerNumber() == playerNumber )
                return p->GetScore();
        }
    }
    return 0;
}

int z0Game::GetPlayerDeaths( int playerNumber ) const
{
    for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
        if ( _shipList[ i ]->IsPlayer() ) {
            Player* p = ( Player* )_shipList[ i ];
            if ( p->GetPlayerNumber() == playerNumber )
                return p->GetDeaths();
        }
    }
    return 0;
}

long z0Game::GetTotalScore() const
{
    std::vector< long > scores;
    for ( int i = 0; i < _players; i++ )
        scores.push_back( 0 );
    for ( std::size_t i = 0; i < _shipList.size(); ++i ) {
        if ( _shipList[ i ]->IsPlayer() ) {
            Player* p = ( Player* )_shipList[ i ];
            scores[ p->GetPlayerNumber() ] = p->GetScore();
        }
    }
    long total = 0;
    for ( int i = 0; i < _players; i++ )
        total += scores[ i ];
    return total;
}

bool z0Game::IsHighScore() const
{
    if ( Player::IsReplaying() )
        return false;
    if ( IsBossMode() )
        return _overmind->GetKilledBosses() >= 6 && _overmind->GetElapsedTime() != 0 &&
        ( _overmind->GetElapsedTime() < _highScores[ Lib::PLAYERS ][ _players - 1 ].second || _highScores[ Lib::PLAYERS ][ _players - 1 ].second == 0 );

    int n = IsWhatMode() ? 3 * Lib::PLAYERS + _players : IsFastMode() ? 2 * Lib::PLAYERS + _players : IsHardMode() ? Lib::PLAYERS + _players : _players - 1;
    const Lib::HighScoreList& list = _highScores[ n ];
    for ( unsigned int i = 0; i < list.size(); i++ ) {
        if ( GetTotalScore() > list[ i ].second )
            return true;
    }
    return false;
}
