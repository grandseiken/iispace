#ifndef z0GAME_H
#define z0GAME_H

#include "game.h"
#include "lib.h"

class z0Game : public Game {
public:

    // Constants
    //------------------------------
    enum GameState {
        STATE_MENU,
        STATE_PAUSE,
        STATE_GAME,
        STATE_HIGHSCORE
    };
    enum BossList {
        BOSS_1A = 1,
        BOSS_1B = 2,
        BOSS_1C = 4,
        BOSS_2A = 8,
        BOSS_2B = 16,
        BOSS_2C = 32
    };

    static const Colour PANEL_TEXT  = 0xeeeeeeff;
    static const Colour PANEL_TRAN  = 0xeeeeee99;
    static const Colour PANEL_BACK  = 0x000000ff;
    static const int STARTING_LIVES = 2;
    static const int BOSSMODE_LIVES = 1;
    #define ALLOWED_CHARS "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! "

    typedef std::vector< Ship* > ShipList;

    z0Game( Lib& lib );
    virtual ~z0Game();

    // Main functions
    //------------------------------
    virtual void Update();
    virtual void Render();
    GameState GetState() const
    { return _state; }

    // Ships
    //------------------------------
    void AddShip( Ship* ship );
    ShipList GetCollisionList( const Vec2& point, int category ) const;
    ShipList GetShipsInRadius( const Vec2& point, float radius ) const;
    bool     AnyCollisionList( const Vec2& point, int category ) const;
 
    // Players
    //------------------------------
    int CountPlayers() const
    { return _players; }
    Player* GetNearestPlayer( const Vec2& point ) const;
    ShipList GetPlayers() const
    { return _playerList; }
    bool IsBossMode() const
    { return _bossMode; }
    void SetBossKilled( BossList boss )
    { _bossesKilled |= boss; }
    void RenderHPBar( float fill )
    { _showHPBar = true; _fillHPBar = fill; }

    // Lives
    //------------------------------
    void AddLife()
    { _lives++; }
    void SubLife()
    { if ( _lives ) _lives--; }
    int GetLives() const
    { return _lives; }

private:

    // Internals
    //------------------------------
    void RenderPanel( const Vec2& low, const Vec2& hi ) const;
    static bool SortShips( Ship* const& a, Ship* const& b );

    bool IsBossModeUnlocked() const
    { return _bossesKilled >= 63; }
    std::string ConvertToTime( long score ) const;

    // Scores
    //------------------------------
    long GetPlayerScore( int playerNumber ) const;
    long GetTotalScore() const;
    bool IsHighScore() const;

    void NewGame( bool bossMode = false );
    void EndGame();

    GameState _state;
    int       _players;
    int       _lives;
    bool      _bossMode;

    bool      _showHPBar;
    float     _fillHPBar;

    int       _selection;
    int       _killTimer;
    int       _exitTimer;

    std::string _enterName;
    int         _enterChar;
    int         _enterR;
    int         _enterTime;
    int         _compliment;

    ShipList  _shipList;
    ShipList  _playerList;
    ShipList  _collisionList;

    int       _controllersConnected;
    bool      _controllersDialog;
    bool      _firstControllersDialog;

    Overmind* _overmind;
    int _bossesKilled;
    Lib::HighScoreTable _highScores;
    std::vector< std::string > _compliments;
};

#endif
