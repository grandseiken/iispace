#ifndef LIB_H
#define LIB_H

#include "z0.h"

class Lib {
public:

    // Constants
    //------------------------------
    enum ExitType {
        NO_EXIT,
        EXIT_TO_LOADER,
        EXIT_TO_SYSTEM,
        EXIT_POWER_OFF
    };

    enum Key {
        KEY_FIRE,
        KEY_BOMB,
        KEY_ACCEPT,
        KEY_CANCEL,
        KEY_MENU,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT
    };

    enum Sound {
        SOUND_PLAYER_DESTROY,
        SOUND_PLAYER_RESPAWN,
        SOUND_PLAYER_FIRE,
        SOUND_PLAYER_SHIELD,
        SOUND_EXPLOSION,
        SOUND_ENEMY_HIT,
        SOUND_ENEMY_DESTROY,
        SOUND_ENEMY_SHATTER,
        SOUND_ENEMY_SPAWN,
        SOUND_BOSS_ATTACK,
        SOUND_BOSS_FIRE,
        SOUND_POWERUP_LIFE,
        SOUND_POWERUP_OTHER,
        SOUND_MENU_CLICK,
        SOUND_MENU_ACCEPT
    };

    enum PadType {
        PAD_NONE     = 0,
        PAD_GAMECUBE = 1,
        PAD_WIIMOTE  = 2,
        PAD_CLASSIC  = 4
    };

    struct Settings {
        bool _disableBackground;
        int  _hudCorrection;
    };

    typedef std::pair  < std::string, long > HighScore;
    typedef std::vector< HighScore >         HighScoreList;
    typedef std::vector< HighScoreList >     HighScoreTable;
    struct SaveData {
        int            _bossesKilled;
        HighScoreTable _highScores;
    };
    static const unsigned int MAX_NAME_LENGTH  = 17;
    static const unsigned int MAX_SCORE_LENGTH = 10;
    static const unsigned int NUM_HIGH_SCORES  = 8;

    static const int PLAYERS = 4;
    static const int WIDTH   = 640;
    static const int HEIGHT  = 480;
    static const int TEXT_WIDTH  = 16;
    static const int TEXT_HEIGHT = 16;
    static const int SOUND_TIMER = 4;

    static bool  ScoreSort( const HighScore& a, const HighScore& b )
    { return a.second > b.second; }

    // General
    //------------------------------
    virtual void Init() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual void Exit( ExitType t ) = 0;
    virtual ExitType GetExitType() const = 0;

    virtual int   RandInt( int lessThan ) const { return 0; }
    virtual float RandFloat() const { return 0.0f; }

    virtual Settings LoadSettings() = 0;
    virtual SaveData LoadSaveData( int version ) = 0;
    virtual void     SaveSaveData( const SaveData& version0, const SaveData& version1 ) = 0;
    virtual bool     Connect() = 0;
    virtual void     Disconnect() = 0;
    virtual void     SendHighScores( const HighScoreTable& table ) = 0;

    // Input
    //------------------------------
    virtual PadType IsPadConnected( int player ) = 0;

    virtual bool IsKeyPressed ( int player, Key k ) = 0;
    virtual bool IsKeyReleased( int player, Key k ) = 0;
    virtual bool IsKeyHeld    ( int player, Key k ) = 0;

    virtual bool IsKeyPressed ( Key k );
    virtual bool IsKeyReleased( Key k );
    virtual bool IsKeyHeld    ( Key k );

    virtual Vec2 GetMoveVelocity ( int player ) = 0;
    virtual Vec2 GetFireTarget   ( int player, const Vec2& position ) = 0;

    // Output
    //------------------------------
    virtual void ClearScreen() = 0;
    virtual void RenderLine( const Vec2& a, const Vec2& b, Colour c ) = 0;
    virtual void RenderText( const Vec2& v, const std::string& text, Colour c ) = 0;
    virtual void RenderRect( const Vec2& low, const Vec2& hi, Colour c, int lineWidth = 0 ) = 0;

    virtual void Rumble( int player, int time ) = 0;
    virtual void StopRumble() = 0;
    virtual bool PlaySound( Sound sound, float volume = 1.0f, float pan = 0.0f, float repitch = 0.0f ) = 0;

    virtual void TakeScreenShot() = 0;

};

#endif
