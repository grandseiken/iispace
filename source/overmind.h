#ifndef OVERMIND_H
#define OVERMIND_H

#include "z0.h"

class Overmind {
public:

    static const int TIMER = 2800;
    static const int POWERUP_TIME = 1200;

    static const int INITIAL_POWER        = 12;
    static const int INITIAL_TRIGGERVAL   = -1;
    static const int LEVELS_PER_GROUP     = 4;
    static const int BASE_GROUPS_PER_BOSS = 4;

    Overmind( z0Game& z0 );
    ~Overmind() { }

    // General
    //------------------------------
    int GetPower() const
    { return _power; }
    void Reset();

    void Update();
    int  GetKilledBosses() const
    { return _bossMod - 1; }
    long GetElapsedTime() const
    { return _elapsedTime + ( _timeStopped ? 0 : time( 0 ) - _startTime ); }
    void StopTime()
    { _elapsedTime += time( 0 ) - _startTime; _timeStopped = true; }
    void UnstopTime()
    { _startTime = time( 0 ) - 1; _timeStopped = false; }

    // Enemy-counting
    //------------------------------
    void OnEnemyDestroy()
    { _count--; }
    void OnEnemyCreate()
    { _count++; }
    int CountEnemies() const
    { return _count; }

private:

    // Individual functions
    //------------------------------
    void SpawnPowerup();
    void SpawnBossReward();

    static void SpawnFollow   ( int num, int div );
    static void SpawnChaser   ( int num, int div );
    static void SpawnSquare   ( int num, int div );
    static void SpawnWall     ( int num, int div );
    static void SpawnFollowHub( int num, int div );
    static void SpawnShielder ( int num, int div );
    static void SpawnTractor  ( bool top, int num, int div );

    // Internals
    //------------------------------
    static void Spawn( Ship* ship );
    static Vec2 SpawnPoint( bool top, int row, int num, int div );

    int RandInt( int lessThan ) const;
    static int HackRandInt( int lessThan );

    void Wave();
    void Boss();
    void BossModeBoss();

    z0Game& _z0;
    int  _power;
    int  _timer;
    int  _count;
    int  _countTrigger;
    int  _levelsMod;
    int  _groupsMod;
    int  _bossMod;
    bool _isBossNext;
    bool _isBossLevel;
    long _startTime;
    long _elapsedTime;
    bool _timeStopped;

    std::vector< int > _boss1Queue;
    std::vector< int > _boss2Queue;

    typedef std::pair< int, int > FormationCost;
    typedef FormationCost ( * SpawnFormationFunction )( bool query, int row );
    typedef std::pair< FormationCost, SpawnFormationFunction > Formation;
    typedef std::vector< Formation > FormationList;

    FormationList _formations;
    void AddFormation( SpawnFormationFunction function );
    static bool SortFormations( const Formation& a, const Formation& b )
    { return a.first.first < b.first.first; }

    // Formation hacks
    //------------------------------
    static int _tRow;
    static z0Game* _tz0;
    void AddFormations();
    #define FORM_DEC( name ) static FormationCost name ( bool query, int row )
    #define FORM_USE( name ) AddFormation( & name )
    #define FORM_DEF( name, cost, minResource ) Overmind::FormationCost Overmind:: name ( bool query, int row ) { if ( query ) return FormationCost( cost , minResource ); _tRow = row;
    #define FORM_END return FormationCost( 0, 0 ); }

    // Formations
    //------------------------------
    FORM_DEC( Square1 );
    FORM_DEC( Square2 );
    FORM_DEC( Square3 );
    FORM_DEC( Wall1 );
    FORM_DEC( Wall2 );
    FORM_DEC( Wall3 );
    FORM_DEC( Follow1 );
    FORM_DEC( Follow2 );
    FORM_DEC( Follow3 );
    FORM_DEC( Chaser1 );
    FORM_DEC( Chaser2 );
    FORM_DEC( Chaser3 );
    FORM_DEC( Chaser4 );
    FORM_DEC( Hub1 );
    FORM_DEC( Hub2 );
    FORM_DEC( Mixed1 );
    FORM_DEC( Mixed2 );
    FORM_DEC( Mixed3 );
    FORM_DEC( Mixed4 );
    FORM_DEC( Mixed5 );
    FORM_DEC( Mixed6 );
    FORM_DEC( Mixed7 );
    FORM_DEC( Tractor1 );
    FORM_DEC( Tractor2 );
    FORM_DEC( Shielder1 );

};

#endif
