#ifndef BOSS_H
#define BOSS_H

#include "ship.h"

// Generic boss
//------------------------------
class Boss : public Ship {
public:

    static const float HP_PER_EXTRA_PLAYER = 0.1f;
    static const float HP_PER_EXTRA_CYCLE  = 0.5f;

    Boss( const Vec2& position, z0Game::BossList boss, int hp, int players, int cycle = 0 );
    virtual ~Boss() { }

    virtual bool IsEnemy() const
    { return true; }
    virtual bool IsBoss() const
    { return true; }
    void SetKilled()
    { SetBossKilled( _flag ); }
    long GetScore()
    { return _score; }
    int GetRemainingHP() const
    { return _hp / 30; }

    void SetIgnoreDamageColourIndex( int shapeIndex )
    { _ignoreDamageColour = shapeIndex; }
    void RenderHPBar();

    // Generic behaviour
    //------------------------------
    virtual void Damage( int damage, Player* source );
    virtual void Render();
    virtual int GetDamage( int damage ) = 0;
    virtual void OnDestroy();

private:

    z0Game::BossList _flag;
    int _hp;
    long _score;
    int _damaged;
    int _ignoreDamageColour;
    int  _maxHp;
    bool _showHp;

};

// Big square boss
//------------------------------
class BigSquareBoss : public Boss
{
public:

    static const int   BASE_HP       = 400;
    static const float SPEED         = 2.5f;
    static const int   TIMER         = 100;
    static const int   STIMER        = 80;
    static const float ATTACK_RADIUS = 120.0f;
    static const int   ATTACK_TIME   = 90;

    BigSquareBoss( int players, int cycle );
    virtual ~BigSquareBoss() { }

    virtual void Update();
    virtual void Render();
    virtual int GetDamage( int damage );

private:

    Vec2    _dir;
    bool    _reverse;
    int     _timer;
    int     _spawnTimer;
    int     _specialTimer;
    bool    _specialAttack;
    Player* _attackPlayer;

};

// Shield bomb boss
//------------------------------
class ShieldBombBoss : public Boss
{
public:

    static const int BASE_HP       = 300;
    static const int TIMER         = 100;
    static const int UNSHIELD_TIME = 300;
    static const int ATTACK_TIME   = 80;
    static const float SPEED = 1.0f;

    ShieldBombBoss( int players, int cycle );
    virtual ~ShieldBombBoss() { }

    virtual void Update();
    virtual int GetDamage( int damage );

private:

    int _timer;
    int _count;
    int _unshielded;
    int _attack;
    bool _side;
    Vec2 _attackDir;

};

// Chaser boss
//------------------------------
class ChaserBoss : public Boss
{
public:

    static const int   BASE_HP = 180;
    static const float SPEED   = 4.0f;
    static const int   TIMER   = 60;
    static const int   MAX_SPLIT = 7;

    ChaserBoss( int players, int cycle, int split = 0, const Vec2& position = Vec2(), int time = TIMER );
    virtual ~ChaserBoss() { }

    virtual void Update();
    virtual void Render();
    virtual int  GetDamage( int damage );
    virtual void OnDestroy();

private:

    bool _onScreen;
    bool _move;
    int  _timer;
    Vec2 _dir;

    int _players;
    int _cycle;
    int _split;

    static int _sharedHp;

};

// Tractor boss
//------------------------------
class TractorBoss : public Boss
{
public:

    static const int   BASE_HP = 900;
    static const float SPEED   = 2.0f;
    static const int   TIMER   = 100;

    TractorBoss( int players, int cycle );
    virtual ~TractorBoss() { }

    virtual void Update();
    virtual void Render();
    virtual int GetDamage( int damage );

private:

    CompoundShape* _s1;
    CompoundShape* _s2;
    Polygon*       _sAttack;
    bool _willAttack;
    bool _stopped;
    bool _generating;
    bool _attacking;
    bool _continue;
    int  _timer;
    int  _attackSize;

    std::vector< Vec2 > _targets;

};

// Ghost boss
//------------------------------
class GhostBoss : public Boss
{
public:

    static const int BASE_HP     = 720;
    static const int TIMER       = 250;
    static const int ATTACK_TIME = 100;

    GhostBoss( int players, int cycle );
    virtual ~GhostBoss() { }

    virtual void Update();
    virtual void Render();
    virtual int GetDamage( int damage );

private:

    bool _visible;
    int  _vTime;
    int  _timer;
    int  _attackTime;
    int  _attack;
    bool _rDir;
    int  _startTime;

};

// Death ray boss
//------------------------------
class DeathRayBoss : public Boss
{
public:

    static const int   BASE_HP    = 500;
    static const int   ARM_HP     = 100;
    static const int   ARM_ATIMER = 300;
    static const int   ARM_RTIMER = 400;
    static const float ARM_SPEED  = 4.0f;
    static const int   RAY_TIMER  = 100;
    static const int   TIMER      = 100;
    static const float SPEED      = 5.0f;

    DeathRayBoss( int players, int cycle );
    virtual ~DeathRayBoss() { }

    virtual void Update();
    virtual void Render();
    virtual int GetDamage( int damage );

    void OnArmDeath( Ship* arm );

private:

    int  _timer;
    bool _laser;
    bool _dir;
    int  _pos;
    z0Game::ShipList _arms;
    int _armTimer;

    int  _rayAttackTimer;
    Vec2 _raySrc1;
    Vec2 _raySrc2;
    Vec2 _rayDest;

};

#endif
