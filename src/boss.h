#ifndef IISPACE_SRC_BOSS_H
#define IISPACE_SRC_BOSS_H

#include "ship.h"

// Generic boss
//------------------------------
class Boss : public Ship {
public:

  static const fixed HP_PER_EXTRA_PLAYER;
  static const fixed HP_PER_EXTRA_CYCLE;

  Boss(const vec2& position, z0Game::boss_list boss,
       int hp, int players, int cycle = 0, bool explodeOnDamage = true);

  void SetKilled()
  {
    z0().set_boss_killed(_flag);
  }

  int64_t GetScore()
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
  void damage(int damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hpBar) const;
  virtual int GetDamage(int damage, bool magic) = 0;
  virtual void OnDestroy();

  static std::vector<std::pair<int, std::pair<vec2, colour_t>>> _fireworks;
  static std::vector<vec2> _warnings;

protected:

  static int CalculateHP(int base, int players, int cycle);

private:

  int32_t _hp;
  int32_t _maxHp;
  z0Game::boss_list _flag;
  int64_t _score;
  int32_t _ignoreDamageColour;
  mutable int _damaged;
  mutable bool _showHp;
  bool _explodeOnDamage;

};

// Big square boss
//------------------------------
class BigSquareBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const fixed SPEED;
  static const int32_t TIMER;
  static const int32_t STIMER;
  static const fixed ATTACK_RADIUS;
  static const int32_t ATTACK_TIME;

  BigSquareBoss(int players, int cycle);

  void update() override;
  void render() const override;
  int GetDamage(int damage, bool magic) override;

private:

  vec2 _dir;
  bool _reverse;
  int32_t _timer;
  int32_t _spawnTimer;
  int32_t _specialTimer;
  bool _specialAttack;
  bool _specialAttackRotate;
  Player* _attackPlayer;

};

// Shield bomb boss
//------------------------------
class ShieldBombBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const int32_t TIMER;
  static const int32_t UNSHIELD_TIME;
  static const int32_t ATTACK_TIME;
  static const fixed SPEED;

  ShieldBombBoss(int players, int cycle);

  void update() override;
  int GetDamage(int damage, bool magic) override;

private:

  int32_t _timer;
  int32_t _count;
  int32_t _unshielded;
  int32_t _attack;
  bool _side;
  vec2 _attackDir;
  bool _shotAlternate;

};

// Chaser boss
//------------------------------
class ChaserBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const fixed SPEED;
  static const int32_t TIMER;
  static const int32_t MAX_SPLIT;

  ChaserBoss(int players, int cycle, int split = 0,
             const vec2& position = vec2(), int time = TIMER, int stagger = 0);

  void update() override;
  void render() const override;
  int GetDamage(int damage, bool magic) override;
  void OnDestroy() override;

  static bool _hasCounted;

private:

  bool _onScreen;
  bool _move;
  int32_t _timer;
  vec2 _dir;
  vec2 _lastDir;

  int32_t _players;
  int32_t _cycle;
  int32_t _split;

  int32_t _stagger;
  static int32_t _count;
  static int32_t _sharedHp;

};

// Tractor boss
//------------------------------
class TractorBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const fixed SPEED;
  static const int32_t TIMER;

  TractorBoss(int players, int cycle);

  void update() override;
  void render() const override;
  int GetDamage(int damage, bool magic) override;

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
  int32_t _shootType;
  bool _sound;
  int32_t _timer;
  int32_t _attackSize;
  std::size_t _attackShapes;

  std::vector<vec2> _targets;

};

// Ghost boss
//------------------------------
class GhostBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const int32_t TIMER;
  static const int32_t ATTACK_TIME;

  GhostBoss(int players, int cycle);

  void update() override;
  void render() const override;
  int GetDamage(int damage, bool magic) override;

private:

  bool _visible;
  int32_t _vTime;
  int32_t _timer;
  int32_t _attackTime;
  int32_t _attack;
  bool _rDir;
  int32_t _startTime;
  int32_t _dangerCircle;
  int32_t _dangerOffset1;
  int32_t _dangerOffset2;
  int32_t _dangerOffset3;
  int32_t _dangerOffset4;
  bool _shotType;
};

// Death ray boss
//------------------------------
class DeathRayBoss : public Boss {
public:

  static const int32_t BASE_HP;
  static const int32_t ARM_HP;
  static const int32_t ARM_ATIMER;
  static const int32_t ARM_RTIMER;
  static const fixed ARM_SPEED;
  static const int32_t RAY_TIMER;
  static const int32_t TIMER;
  static const fixed SPEED;

  DeathRayBoss(int players, int cycle);

  void update() override;
  void render() const override;
  int GetDamage(int damage, bool magic) override;

  void OnArmDeath(Ship* arm);

private:

  int32_t _timer;
  bool _laser;
  bool _dir;
  int _pos;
  z0Game::ShipList _arms;
  int32_t _armTimer;
  int32_t _shotTimer;

  int32_t _rayAttackTimer;
  vec2 _raySrc1;
  vec2 _raySrc2;
  vec2 _rayDest;

  std::vector<std::pair<int32_t, int32_t>> _shotQueue;

};

// Super boss
//------------------------------
class SuperBossArc : public Boss {
public:

  SuperBossArc(const vec2& position, int players, int cycle,
               int i, Ship* boss, int timer = 0);

  void update() override;
  int GetDamage(int damage, bool magic) override;
  void OnDestroy() override;
  void render() const override;

  int GetTimer() const
  {
    return _timer;
  }

private:

  Ship* _boss;
  int32_t _i;
  int32_t _timer;
  int32_t _sTimer;

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

  static const int32_t BASE_HP;
  static const int32_t ARC_HP;

  SuperBoss(int players, int cycle);

  void update() override;
  int GetDamage(int damage, bool magic) override;
  void OnDestroy() override;

private:

  int32_t _players;
  int32_t _cycle;
  int32_t _cTimer;
  int32_t _timer;
  std::vector<bool> _destroyed;
  std::vector<SuperBossArc*> _arcs;
  State _state;
  int32_t _snakes;

};

#endif
