#ifndef BOSS_H
#define BOSS_H

#include "ship.h"

// Generic boss
//------------------------------
class Boss : public Ship {
public:

  static const fixed HP_PER_EXTRA_PLAYER;
  static const fixed HP_PER_EXTRA_CYCLE;

  Boss(const Vec2& position, z0Game::BossList boss,
       int hp, int players, int cycle = 0, bool explodeOnDamage = true);

  virtual ~Boss() {}

  virtual bool IsEnemy() const
  {
    return true;
  }

  void SetKilled()
  {
    SetBossKilled(_flag);
  }

  long GetScore()
  {
    return _score;
  }

  int GetRemainingHP() const
  {
    return _hp / 30;
  }

  int GetMaxHP() const
  {
    return _maxHp / 30;
  }

  void RestoreHP(int hp)
  {
    _hp += hp;
  }

  bool IsHPLow() const
  {
    return (_maxHp * 1.2f) / _hp >= 3;
  }

  void SetIgnoreDamageColourIndex(int shapeIndex)
  {
    _ignoreDamageColour = shapeIndex;
  }

  void RenderHPBar() const;

  // Generic behaviour
  //------------------------------
  virtual void Damage(int damage, bool magic, Player* source);
  virtual void Render() const;
  void Render(bool hpBar) const;
  virtual int GetDamage(int damage, bool magic) = 0;
  virtual void OnDestroy();

  static std::vector<std::pair<int, std::pair<Vec2, Colour>>> _fireworks;
  static std::vector<Vec2> _warnings;

protected:

  static int CalculateHP(int base, int players, int cycle);

private:

  int _hp;
  int _maxHp;
  z0Game::BossList _flag;
  long _score;
  int _ignoreDamageColour;
  mutable int _damaged;
  mutable bool _showHp;
  bool _explodeOnDamage;

};

// Big square boss
//------------------------------
class BigSquareBoss : public Boss {
public:

  static const int BASE_HP;
  static const fixed SPEED;
  static const int TIMER;
  static const int STIMER;
  static const fixed ATTACK_RADIUS;
  static const int ATTACK_TIME;

  BigSquareBoss(int players, int cycle);
  virtual ~BigSquareBoss() {}

  virtual void Update();
  virtual void Render() const;
  virtual int GetDamage(int damage, bool magic);

private:

  Vec2 _dir;
  bool _reverse;
  int _timer;
  int _spawnTimer;
  int _specialTimer;
  bool _specialAttack;
  bool _specialAttackRotate;
  Player* _attackPlayer;

};

// Shield bomb boss
//------------------------------
class ShieldBombBoss : public Boss {
public:

  static const int BASE_HP;
  static const int TIMER;
  static const int UNSHIELD_TIME;
  static const int ATTACK_TIME;
  static const fixed SPEED;

  ShieldBombBoss(int players, int cycle);
  virtual ~ShieldBombBoss() {}

  virtual void Update();
  virtual int GetDamage(int damage, bool magic);

private:

  int _timer;
  int _count;
  int _unshielded;
  int _attack;
  bool _side;
  Vec2 _attackDir;
  bool _shotAlternate;

};

// Chaser boss
//------------------------------
class ChaserBoss : public Boss {
public:

  static const int BASE_HP;
  static const fixed SPEED;
  static const int TIMER;
  static const int MAX_SPLIT;

  ChaserBoss(int players, int cycle, int split = 0,
             const Vec2& position = Vec2(), int time = TIMER, int stagger = 0);
  virtual ~ChaserBoss() {}

  virtual void Update();
  virtual void Render() const;
  virtual int  GetDamage(int damage, bool magic);
  virtual void OnDestroy();

  static bool _hasCounted;

private:

  bool _onScreen;
  bool _move;
  int _timer;
  Vec2 _dir;
  Vec2 _lastDir;

  int _players;
  int _cycle;
  int _split;

  int _stagger;
  static int _count;
  static int _sharedHp;

};

// Tractor boss
//------------------------------
class TractorBoss : public Boss {
public:

  static const int BASE_HP;
  static const fixed SPEED;
  static const int TIMER;

  TractorBoss(int players, int cycle);
  virtual ~TractorBoss() {}

  virtual void Update();
  virtual void Render() const;
  virtual int GetDamage(int damage, bool magic);

private:

  CompoundShape* _s1;
  CompoundShape* _s2;
  Polygon* _sAttack;
  bool _willAttack;
  bool _stopped;
  bool _generating;
  bool _attacking;
  bool _continue;
  bool _genDir;
  int _shootType;
  bool _sound;
  int _timer;
  int _attackSize;
  int _attackShapes;

  std::vector<Vec2> _targets;

};

// Ghost boss
//------------------------------
class GhostBoss : public Boss {
public:

  static const int BASE_HP;
  static const int TIMER;
  static const int ATTACK_TIME;

  GhostBoss(int players, int cycle);
  virtual ~GhostBoss() {}

  virtual void Update();
  virtual void Render() const;
  virtual int GetDamage(int damage, bool magic);

private:

  bool _visible;
  int _vTime;
  int _timer;
  int _attackTime;
  int _attack;
  bool _rDir;
  int _startTime;
  int _dangerCircle;
  int _dangerOffset1;
  int _dangerOffset2;
  int _dangerOffset3;
  int _dangerOffset4;
  bool _shotType;
};

// Death ray boss
//------------------------------
class DeathRayBoss : public Boss {
public:

  static const int BASE_HP;
  static const int ARM_HP;
  static const int ARM_ATIMER;
  static const int ARM_RTIMER;
  static const fixed ARM_SPEED;
  static const int RAY_TIMER;
  static const int TIMER;
  static const fixed SPEED;

  DeathRayBoss(int players, int cycle);
  virtual ~DeathRayBoss() {}

  virtual void Update();
  virtual void Render() const;
  virtual int GetDamage(int damage, bool magic);

  void OnArmDeath(Ship* arm);

private:

  int _timer;
  bool _laser;
  bool _dir;
  int _pos;
  z0Game::ShipList _arms;
  int _armTimer;
  int _shotTimer;

  int _rayAttackTimer;
  Vec2 _raySrc1;
  Vec2 _raySrc2;
  Vec2 _rayDest;

  std::vector<std::pair<int, int>> _shotQueue;

};

// Super boss
//------------------------------
class SuperBossArc : public Boss {
public:

  SuperBossArc(const Vec2& position, int players, int cycle,
               int i, Ship* boss, int timer = 0);
  virtual ~SuperBossArc() {}

  virtual void Update();
  virtual int GetDamage(int damage, bool magic);
  virtual void OnDestroy();
  virtual void Render() const;

  int GetTimer() const
  {
    return _timer;
  }

private:

  Ship* _boss;
  int _i;
  int _timer;
  int _sTimer;

};

class SuperBoss : public Boss {
  friend class SuperBossArc;
  friend class RainbowShot;
public:

  enum State {
    STATE_ARRIVE,
    STATE_IDLE,
    STATE_ATTACK
  };

  static const int BASE_HP;
  static const int ARC_HP;

  SuperBoss(int players, int cycle);
  virtual ~SuperBoss() {}

  virtual void Update();
  virtual int GetDamage(int damage, bool magic);
  virtual void OnDestroy();

private:

  int _players;
  int _cycle;
  int _cTimer;
  int _timer;
  std::vector<bool> _destroyed;
  std::vector<SuperBossArc*> _arcs;
  State _state;
  int _snakes;

};

#endif
