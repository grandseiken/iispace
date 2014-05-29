#ifndef IISPACE_SRC_ENEMY_H
#define IISPACE_SRC_ENEMY_H

#include "ship.h"

// Basic enemy
//------------------------------
class Enemy : public Ship {
public:

  Enemy(const vec2& position, Ship::ship_category type,
        int hp, bool explodeOnDestroy = true);

  long GetScore() const
  {
    return _score;
  }

  void SetScore(long score)
  {
    _score = score;
  }

  int GetHP() const
  {
    return _hp;
  }

  void SetDestroySound(Lib::Sound sound)
  {
    _destroySound = sound;
  }

  // Generic behaviour
  //------------------------------
  void damage(int damage, bool magic, Player* source) override;
  void render() const override;
  virtual void OnDestroy(bool bomb) {}

private:

  int _hp;
  long _score;
  mutable int _damaged;
  Lib::Sound _destroySound;
  bool _explodeOnDestroy;

};

// Follower enemy
//------------------------------
class Follow : public Enemy {
public:

  static const int TIME;
  static const fixed SPEED;

  Follow(const vec2& position, fixed radius = 10, int hp = 1);
  void update() override;

private:

  int   _timer;
  Ship* _target;

};

// Chaser enemy
//------------------------------
class Chaser : public Enemy {
public:

  static const int TIME;
  static const fixed SPEED;

  Chaser(const vec2& position);
  void update() override;

private:

  bool _move;
  int _timer;
  vec2 _dir;

};

// Square enemy
//------------------------------
class Square : public Enemy {
public:

  static const fixed SPEED;

  Square(const vec2& position, fixed rotation = fixed::pi / 2);
  void update() override;
  void render() const override;

private:

  vec2 _dir;
  int _timer;

};

// Wall enemy
//------------------------------
class Wall : public Enemy {
public:

  static const int TIMER;
  static const fixed SPEED;

  Wall(const vec2& position, bool rdir);
  void update() override;
  void OnDestroy(bool bomb) override;

private:

  vec2 _dir;
  int  _timer;
  bool _rotate;
  bool _rdir;

};

// Follower-spawning enemy
//------------------------------
class FollowHub : public Enemy {
public:

  static const int TIMER;
  static const fixed SPEED;

  FollowHub(const vec2& position, bool powerA = false, bool powerB = false);
  void update() override;
  void OnDestroy(bool bomb) override;

private:

  int _timer;
  vec2 _dir;
  int _count;
  bool _powerA;
  bool _powerB;

};

// Shielder enemy
//------------------------------
class Shielder : public Enemy {
public:

  static const fixed SPEED;
  static const int TIMER;

  Shielder(const vec2& position, bool power = false);
  void update() override;

private:

  vec2 _dir;
  int _timer;
  bool _rotate;
  bool _rDir;
  bool _power;

};

// Tractor beam enemy
//------------------------------
class Tractor : public Enemy {
public:

  static const int TIMER;
  static const fixed SPEED;
  static const fixed TRACTOR_SPEED;

  Tractor(const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:

  int  _timer;
  vec2 _dir;
  bool _power;

  bool _ready;
  bool _spinning;
  z0Game::ShipList _players;

};

#endif
