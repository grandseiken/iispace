#include "z0Game.h"
#include "lib.h"
#include "player.h"
#include "enemy.h"
#include "overmind.h"

z0Game::z0Game( Lib& lib )
: Game( lib )
, _state( STATE_MENU )
, _players( 1 )
, _lives( 0 )
, _bossMode( false )
, _showHPBar( false )
, _fillHPBar( 0 )
, _selection( 0 )
, _killTimer( 0 )
, _exitTimer( 0 )
, _enterChar( 0 )
, _enterTime( 0 )
, _controllersConnected( 0 )
, _controllersDialog( false )
, _firstControllersDialog( false )
, _overmind( 0 )
, _bossesKilled( 0 )
{
    _overmind = new Overmind( *this );
    Lib::SaveData save = lib.LoadSaveData();
    _highScores = save._highScores;
    _bossesKilled = save._bossesKilled;

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
}

z0Game::~z0Game()
{
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
        if ( _shipList[ i ]->IsEnemy() )
            _overmind->OnEnemyDestroy();
        delete _shipList[ i ];
    }

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

    // Main menu
    //------------------------------
    if ( _state == STATE_MENU ) {

        if ( lib.IsKeyPressed( Lib::KEY_UP ) ) {
            _selection--;
            if ( _selection < 0 && !IsBossModeUnlocked() )
                _selection =  0;
            else if ( _selection < -1 &&  IsBossModeUnlocked() )
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
            if ( _selection == 0 ) {
                NewGame();
            }
            else if ( _selection == 1 ) {
                _players++;
                if ( _players > Lib::PLAYERS ) _players = 1;
            }
            else if ( _selection == 2 ) {
                _exitTimer = 10;
            }
            else if ( _selection == -1 ) {
                NewGame( true );
            }

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
        }

    }
    // Paused
    //------------------------------
    else if ( _state == STATE_PAUSE ) {

        int t = _selection;
        if ( lib.IsKeyPressed( Lib::KEY_UP ) && _selection > 0 )
            _selection--;
        if ( lib.IsKeyPressed( Lib::KEY_DOWN ) && _selection < 1 )
            _selection++;
        if ( t != _selection )
            GetLib().PlaySound( Lib::SOUND_MENU_CLICK );

        if ( lib.IsKeyPressed( Lib::KEY_ACCEPT ) || lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            if ( _selection == 0 ) {
                _state = STATE_GAME;
                _overmind->UnstopTime();
            }
            else if ( _selection == 1 ) {
                _state = STATE_HIGHSCORE;
                _enterChar = 0;
                _compliment = lib.RandInt( _compliments.size() );
                _killTimer = 0;
            }
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }
        if ( lib.IsKeyPressed( Lib::KEY_CANCEL ) ) {
            _state = STATE_GAME;
            _overmind->UnstopTime();
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }

    }
    // In-game
    //------------------------------
    else if ( _state == STATE_GAME ) {

        int controllers = 0;
        for ( int i = 0; i < CountPlayers(); i++ ) {
            if ( lib.IsPadConnected( i ) )
                controllers++;
        }
        if ( controllers < _controllersConnected && !_controllersDialog ) {
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
            _overmind->StopTime();
            _state = STATE_PAUSE;
            _selection = 0;
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }

        Player::UpdateFireTimer();
        std::sort( _collisionList.begin(), _collisionList.end(), &SortShips );
        for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
            if ( !_shipList[ i ]->IsDestroyed() )
                _shipList[ i ]->Update();
        }


        for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
            if ( _shipList[ i ]->IsDestroyed() ) {
                if ( _shipList[ i ]->IsEnemy() )
                    _overmind->OnEnemyDestroy();

                for ( unsigned int j = 0; j < _collisionList.size(); j++ )
                    if ( _collisionList[ j ] == _shipList[ i ] )
                        _collisionList.erase( _collisionList.begin() + j );

                delete _shipList[ i ];
                _shipList.erase( _shipList.begin() + i );
                i--;
            }
        }
        _overmind->Update();

        if ( ( Player::CountKilledPlayers() == _players && !GetLives() || ( IsBossMode() && _overmind->GetKilledBosses() >= 6 ) ) && !_killTimer ) {
            _killTimer = 100;
            _overmind->StopTime();
        }
        if ( _killTimer ) {
            _killTimer--;
            if ( !_killTimer ) {
                _state = STATE_HIGHSCORE;
                _enterChar = 0;
                _compliment = lib.RandInt( _compliments.size() );
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
            }
        }

    }
    // Entering high-score
    //------------------------------
    else if ( _state == STATE_HIGHSCORE )
    {
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
                if ( _enterChar >= int( chars.length() ) ) _enterChar -= chars.length();
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyHeld( Lib::KEY_RIGHT ) ) {
                _enterR++;
                _enterTime = 16;
                if ( _enterR % 5 == 0 && _enterR > 5 ) {
                    _enterChar++;
                    if ( _enterChar >= int( chars.length() ) ) _enterChar -= chars.length();
                    GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
                }
            }
            if ( lib.IsKeyPressed( Lib::KEY_LEFT ) ) {
                _enterR = 0;
                _enterChar--;
                if ( _enterChar < 0 ) _enterChar += chars.length();
                GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
            }
            if ( lib.IsKeyHeld( Lib::KEY_LEFT ) ) {
                _enterR++;
                _enterTime = 16;
                if ( _enterR % 5 == 0 && _enterR > 5 ) {
                    _enterChar--;
                    if ( _enterChar < 0 ) _enterChar += chars.length();
                    GetLib().PlaySound( Lib::SOUND_MENU_CLICK );
                }
            }
            if ( lib.IsKeyPressed( Lib::KEY_MENU ) ) {
                GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
                if ( !IsBossMode() ) {
                    Lib::HighScoreList& list = _highScores[ _players - 1 ];
                    list.push_back( Lib::HighScore( _enterName, GetTotalScore() ) );
                    std::sort( list.begin(), list.end(), &Lib::ScoreSort );
                    list.erase( list.begin() + ( list.size() - 1 ) );
                    EndGame();
                } else {
                    long score = _overmind->GetElapsedTime();
                    score -= 10 * GetLives();
                    if ( score <= 0 )
                        score = 1;
                    _highScores[ Lib::PLAYERS ][ _players - 1 ].first = _enterName;
                    _highScores[ Lib::PLAYERS ][ _players - 1 ].second = score;
                    EndGame();
                }
            }
        }
        else if ( lib.IsKeyPressed( Lib::KEY_MENU ) ) {
            EndGame();
            GetLib().PlaySound( Lib::SOUND_MENU_ACCEPT );
        }
    }
}

void z0Game::Render()
{
    Lib& lib = GetLib();

    _showHPBar = false;
    _fillHPBar = 0;

    if ( !_firstControllersDialog ) {
        for ( unsigned int i = CountPlayers(); i < _shipList.size(); i++ ) {
            _shipList[ i ]->Render();
        }
        for ( unsigned int i = 0; i < unsigned( CountPlayers() ) && i < _shipList.size(); i++ ) {
            _shipList[ i ]->Render();
        }
    }

    // In-game
    //------------------------------
    if ( _state == STATE_GAME ) {

        if ( _controllersDialog ) {

            RenderPanel( Vec2( 3, 3 ), Vec2( 32, 8 + 2 * CountPlayers() ) );

            lib.RenderText( Vec2( 4, 4 ),  "CONTROLLERS FOUND",   PANEL_TEXT );

            for ( int i = 0; i < CountPlayers(); i++ ) {
                std::stringstream ss;
                ss << "PLAYER " << ( i + 1 ) << ": ";
                lib.RenderText( Vec2( 4, 8 + 2 * i ), ss.str(), PANEL_TEXT );

                std::stringstream ss2;
                int pads = lib.IsPadConnected( i );
                if ( pads & Lib::PAD_GAMECUBE ) {
                    ss2 << "GAMECUBE";
                    if ( pads & Lib::PAD_WIIMOTE )
                        ss2 << ", ";
                }
                if ( pads & Lib::PAD_WIIMOTE )
                    ss2 << "WIIMOTE";
                if ( !pads )
                    ss2 << "NONE";

                lib.RenderText( Vec2( 14, 8 + 2 * i ), ss2.str(), pads ? Player::GetPlayerColour( i ) : PANEL_TEXT );
            }

            return;
        }

        std::stringstream ss;
        ss << _lives << " live(s)";
        lib.RenderText( Vec2( Lib::WIDTH / ( 2 * Lib::TEXT_WIDTH ) - ss.str().length() / 2, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2 ),
                        ss.str(), PANEL_TRAN );
        if ( IsBossMode() ) {
            std::stringstream sst;
            sst << ConvertToTime( _overmind->GetElapsedTime() );
            lib.RenderText( Vec2( Lib::WIDTH / ( 2 * Lib::TEXT_WIDTH ) - sst.str().length() / 2, 1 ),
                            sst.str(), PANEL_TRAN );
        } else if ( _showHPBar ) {
            lib.RenderRect( Vec2( Lib::WIDTH / 2.0f - 48, 16 ),
                            Vec2( Lib::WIDTH / 2.0f + 48, 32 ), PANEL_TRAN, 2 );
            lib.RenderRect( Vec2( Lib::WIDTH / 2.0f - 44, 20 ),
                            Vec2( Lib::WIDTH / 2.0f - 44 + 88 * _fillHPBar, 28 ), PANEL_TRAN );
        }

    }
    // Main menu screen
    //------------------------------
    else if ( _state == STATE_MENU ) {

        // Main menu
        //------------------------------
        RenderPanel( Vec2( 3, 3 ), Vec2( 19, 14 ) );

        lib.RenderText( Vec2( 37 - 16, 3 ),  "coded by: SEiKEN", PANEL_TEXT );
        lib.RenderText( Vec2( 37 - 16, 4 ),  "stu@seiken.co.uk", PANEL_TEXT );
        lib.RenderText( Vec2( 37 - 9, 6 ),          "-testers-", PANEL_TEXT );
        lib.RenderText( Vec2( 37 - 9, 7 ),          "MATT BELL", PANEL_TEXT );
        lib.RenderText( Vec2( 37 - 9, 8 ),          "RUFUZZZZZ", PANEL_TEXT );

        std::string b = "BOSSES:  ";
        if ( _bossesKilled & BOSS_1A ) b += "X"; else b += "-";
        if ( _bossesKilled & BOSS_1B ) b += "X"; else b += "-";
        if ( _bossesKilled & BOSS_1C ) b += "X"; else b += "-";
        b += " ";
        if ( _bossesKilled & BOSS_2A ) b += "X"; else b += "-";
        if ( _bossesKilled & BOSS_2B ) b += "X"; else b += "-";
        if ( _bossesKilled & BOSS_2C ) b += "X"; else b += "-";
        lib.RenderText( Vec2( 37 - 16, 13 ), b, PANEL_TEXT );

        lib.RenderText( Vec2( 4, 4 ),  "WiiSPACE",   PANEL_TEXT );
        lib.RenderText( Vec2( 6, 8 ),  "START GAME", PANEL_TEXT );
        lib.RenderText( Vec2( 6, 10 ), "PLAYERS",    PANEL_TEXT );
        lib.RenderText( Vec2( 6, 12 ), "EXiT",       PANEL_TEXT );

        if ( IsBossModeUnlocked() )
            lib.RenderText( Vec2( 6, 6 ), "BOSS MODE", PANEL_TEXT );

        Vec2 low( 4 * Lib::TEXT_WIDTH + 4, ( 8 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 );
        Vec2  hi( 5 * Lib::TEXT_WIDTH - 4, ( 9 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 );
        lib.RenderRect( low, hi, PANEL_TEXT, 1 );

        for ( int i = 0; i < _players; i++ ) {
            std::stringstream ss;
            ss << ( i + 1 );
            lib.RenderText( Vec2( 14 + i, 10 ), ss.str(), Player::GetPlayerColour( i ) );
        }

        // High score table
        //------------------------------
        RenderPanel( Vec2( 3, 15 ), Vec2( 37, 27 ) );

        std::string s        = "HiGH SCORES    ";
        s += _selection == -1 ?     "BOSS MODE" :
             _players   ==  1 ?    "ONE PLAYER" :
             _players   ==  2 ?   "TWO PLAYERS" :
             _players   ==  3 ? "THREE PLAYERS" :
             _players   ==  4 ?  "FOUR PLAYERS" : "";
        lib.RenderText( Vec2( 4, 16 ), s, PANEL_TEXT );

        if ( _selection == -1 ) {
            lib.RenderText( Vec2( 4, 18 ), "ONE PLAYER", PANEL_TEXT );
            lib.RenderText( Vec2( 4, 20 ), "TWO PLAYERS", PANEL_TEXT );
            lib.RenderText( Vec2( 4, 22 ), "THREE PLAYERS", PANEL_TEXT );
            lib.RenderText( Vec2( 4, 24 ), "FOUR PLAYERS", PANEL_TEXT );

            for ( int i = 0; i < Lib::PLAYERS; i++ ) {
                std::string score = ConvertToTime( _highScores[ Lib::PLAYERS ][ i ].second );
                if ( score.length() > Lib::MAX_SCORE_LENGTH )
                    score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
                std::string name = _highScores[ Lib::PLAYERS ][ i ].first;
                if ( name.length() > Lib::MAX_NAME_LENGTH )
                    name = name.substr( 0, Lib::MAX_NAME_LENGTH );

                lib.RenderText( Vec2( 19, 18 + i * 2 ), score, PANEL_TEXT );
                lib.RenderText( Vec2( 19, 19 + i * 2 ), name, PANEL_TEXT );
            }
        }
        else {
            for ( unsigned int i = 0; i < Lib::NUM_HIGH_SCORES; i++ ) {
                std::stringstream ssi;
                ssi << ( i + 1 ) << ".";
                lib.RenderText( Vec2( 4, 18 + i ), ssi.str(), PANEL_TEXT );

                if ( _highScores[ _players - 1 ][ i ].second <= 0 )
                    continue;

                std::stringstream ss;
                ss << _highScores[ _players - 1 ][ i ].second;
                std::string score = ss.str();
                if ( score.length() > Lib::MAX_SCORE_LENGTH )
                    score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
                std::string name = _highScores[ _players - 1 ][ i ].first;
                if ( name.length() > Lib::MAX_NAME_LENGTH )
                    name = name.substr( 0, Lib::MAX_NAME_LENGTH );

                lib.RenderText( Vec2( 7, 18 + i ),  score, PANEL_TEXT );
                lib.RenderText( Vec2( 19, 18 + i ), name, PANEL_TEXT );
            }
        }
    }
    // Pause menu
    //------------------------------
    else if ( _state == STATE_PAUSE ) {

        RenderPanel( Vec2( 3, 3 ), Vec2( 15, 12 ) );

        lib.RenderText( Vec2( 4, 4 ),  "PAUSED",   PANEL_TEXT );
        lib.RenderText( Vec2( 6, 8 ),  "CONTINUE", PANEL_TEXT );
        lib.RenderText( Vec2( 6, 10 ), "END GAME", PANEL_TEXT );

        Vec2 low( 4 * Lib::TEXT_WIDTH + 4, ( 8 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 );
        Vec2  hi( 5 * Lib::TEXT_WIDTH - 4, ( 9 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 );
        lib.RenderRect( low, hi, PANEL_TEXT, 1 );
    }
    // Score screen
    //------------------------------
    else if ( _state == STATE_HIGHSCORE ) {

        // Name enter
        //------------------------------
        if ( IsHighScore() ) {
            std::string chars = ALLOWED_CHARS;

            RenderPanel( Vec2( 3, 20 ), Vec2( 28, 27 ) );
            lib.RenderText( Vec2( 4, 21 ), "It's a high score!", PANEL_TEXT );
            lib.RenderText( Vec2( 4, 23 ), _players == 1 ? "Enter name:" : "Enter team name:", PANEL_TEXT );
            lib.RenderText( Vec2( 6, 25 ), _enterName, PANEL_TEXT );
            if ( ( _enterTime / 16 ) % 2 && _enterName.length() < Lib::MAX_NAME_LENGTH ) {
                lib.RenderText( Vec2( 6 + _enterName.length(), 25 ), chars.substr( _enterChar, 1 ), 0xbbbbbbff );
            }
            Vec2 low( 4 * Lib::TEXT_WIDTH + 4, ( 25 + 2 * _selection ) * Lib::TEXT_HEIGHT + 4 );
            Vec2  hi( 5 * Lib::TEXT_WIDTH - 4, ( 26 + 2 * _selection ) * Lib::TEXT_HEIGHT - 4 );
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

            RenderPanel( Vec2( 3, 3 ), Vec2( 28, b ? 10 : 8 ) );
            if ( b ) {
                std::stringstream ss;
                ss << ( extraLives * 10 ) << " second bonus for " << extraLives << " extra li" << ( extraLives == 1 ? "fe" : "ves" ) << ".";
                lib.RenderText( Vec2( 4, 4 ), ss.str(), PANEL_TEXT );
            }

            lib.RenderText( Vec2( 4, b ? 6 : 4 ), "TIME ELAPSED: " + ConvertToTime( score ), PANEL_TEXT );
            std::stringstream ss;
            ss << "BOSS DESTROY: " << _overmind->GetKilledBosses();
            lib.RenderText( Vec2( 4, b ? 8 : 6 ), ss.str(), PANEL_TEXT );
            return;
        }

        // Score listing
        //------------------------------
        RenderPanel( Vec2( 3, 3 ), Vec2( 37, 8 + 2 * _players + ( _players > 1 ? 2 : 0 ) ) );

        std::stringstream ss;
        ss << GetTotalScore();
        std::string score = ss.str();
        if ( score.length() > Lib::MAX_SCORE_LENGTH )
            score = score.substr( 0, Lib::MAX_SCORE_LENGTH );
        lib.RenderText( Vec2( 4, 4 ), "TOTAL SCORE: " + score, PANEL_TEXT );

        for ( int i = 0; i < _players; i++ ) {
            std::stringstream sss;
            sss << GetPlayerScore( i );
            score = sss.str();
            if ( score.length() > Lib::MAX_SCORE_LENGTH )
                score = score.substr( 0, Lib::MAX_SCORE_LENGTH );

            std::stringstream ssp;
            ssp << "PLAYER " << ( i + 1 ) << ":";
            lib.RenderText( Vec2( 4, 8 + 2 * i ),  ssp.str(), PANEL_TEXT );
            lib.RenderText( Vec2( 14, 8 + 2 * i ), score,     Player::GetPlayerColour( i ) );
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
                lib.RenderText( Vec2( 4, 8 + 2 * _players ), s.str(), Player::GetPlayerColour( best ) );

                std::string compliment = _compliments[ _compliment ];
                lib.RenderText( Vec2( 12, 8 + 2 * _players ), compliment, PANEL_TEXT );
            }
            else {
                lib.RenderText( Vec2( 4, 8 + 2 * _players ), "Oh dear!", PANEL_TEXT );
            }
        }

    }
}

// Game control
//------------------------------
void z0Game::NewGame( bool bossMode )
{
    _controllersDialog = true;
    _firstControllersDialog = true;
    _controllersConnected = 0;
    _bossMode = bossMode;
    _overmind->Reset();
    _lives = bossMode ? BOSSMODE_LIVES : STARTING_LIVES;
    for ( int i = 0; i < _players; i++ ) {
        Vec2 v( ( 1 + i ) * Lib::WIDTH / ( 1 + _players ), Lib::HEIGHT / 2 );
        Player* p = new Player( v, i );
        AddShip( p );
        _playerList.push_back( p );
    }
    _state = STATE_GAME;
}

void z0Game::EndGame()
{
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
        if ( _shipList[ i ]->IsEnemy() )
            _overmind->OnEnemyDestroy();
        delete _shipList[ i ];
    }

    _shipList.clear();
    _playerList.clear();
    _collisionList.clear();

    Lib::SaveData save;
    save._highScores = _highScores;
    save._bossesKilled = _bossesKilled;
    GetLib().SaveSaveData( save );
    _state = STATE_MENU;
    _selection = ( IsBossModeUnlocked() && IsBossMode() ) ? -1 : 0;
}

void z0Game::AddShip( Ship* ship )
{
    ship->SetGame( *this );
    if ( ship->IsEnemy() )
        _overmind->OnEnemyCreate();
    _shipList.push_back( ship );

    if ( ship->GetBoundingWidth() > 1.0f )
        _collisionList.push_back( ship );
}

// Ship info
//------------------------------
z0Game::ShipList z0Game::GetCollisionList( const Vec2& point, int category ) const
{
    ShipList r;
    float x = point._x;
    float y = point._y;

    for ( unsigned int i = 0; i < _collisionList.size(); i++ ) {
        float sx = _collisionList[ i ]->GetPosition()._x;
        float sy = _collisionList[ i ]->GetPosition()._y;
        float w  = _collisionList[ i ]->GetBoundingWidth();

        if ( sx - w > x )
            break;
        if ( sx + w < x || sy + w < y || sy - w > y )
            continue;

        if ( _collisionList[ i ]->CheckPoint( point, category ) )
            r.push_back( _collisionList[ i ] );
    }
    return r;
}

z0Game::ShipList z0Game::GetShipsInRadius( const Vec2& point, float radius ) const
{
    ShipList r;
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
        if (  ( _shipList[ i ]->GetPosition() - point ).Length() <= radius )
            r.push_back( _shipList[ i ] );
    }
    return r;
}

bool z0Game::AnyCollisionList( const Vec2& point, int category ) const
{
    float x = point._x;
    float y = point._y;

    for ( unsigned int i = 0; i < _collisionList.size(); i++ ) {
        float sx = _collisionList[ i ]->GetPosition()._x;
        float sy = _collisionList[ i ]->GetPosition()._y;
        float w  = _collisionList[ i ]->GetBoundingWidth();

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
    float d = Lib::WIDTH * Lib::HEIGHT;
    float deadD = Lib::WIDTH * Lib::HEIGHT;

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

bool z0Game::SortShips( Ship* const& a, Ship* const& b )
{
    return a->GetPosition()._x - a->GetBoundingWidth() < b->GetPosition()._x - b->GetBoundingWidth();
}

// UI layout
//------------------------------
void z0Game::RenderPanel( const Vec2& low, const Vec2& hi ) const
{
    Vec2 tlow( low._x * Lib::TEXT_WIDTH, low._y * Lib::TEXT_HEIGHT );
    Vec2  thi(  hi._x * Lib::TEXT_WIDTH,  hi._y * Lib::TEXT_HEIGHT );
    GetLib().RenderRect( tlow, thi, PANEL_BACK );
    GetLib().RenderRect( tlow, thi, PANEL_TEXT, 4 );
}

std::string z0Game::ConvertToTime( long score ) const
{
    if ( score == 0 )
        return "--:--";
    int mins = 0;
    while ( score > 60 && mins < 99 ) {
        score -= 60;
        mins++;
    }
    int secs = score;

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
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
        if ( _shipList[ i ]->IsPlayer() ) {
            Player* p = ( Player* )_shipList[ i ];
            if ( p->GetPlayerNumber() == playerNumber )
                return p->GetScore();
        }
    }
    return 0;
}

long z0Game::GetTotalScore() const
{
    std::vector< long > scores;
    for ( int i = 0; i < _players; i++ )
        scores.push_back( 0 );
    for ( unsigned int i = 0; i < _shipList.size(); i++ ) {
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
    if ( IsBossMode() )
        return _overmind->GetKilledBosses() >= 6 && _overmind->GetElapsedTime() != 0 &&
        ( _overmind->GetElapsedTime() < _highScores[ Lib::PLAYERS ][ _players - 1 ].second || _highScores[ Lib::PLAYERS ][ _players - 1 ].second == 0 );

    const Lib::HighScoreList& list = _highScores[ _players - 1 ];
    for ( unsigned int i = 0; i < list.size(); i++ ) {
        if ( GetTotalScore() > list[ i ].second )
            return true;
    }
    return false;
}
