#ifndef IISPACE_SRC_ENEMY_H
#define IISPACE_SRC_ENEMY_H

#include "ship.h"

// Basic enemy
//------------------------------
class Enemy : public Ship {
public:

  Enemy(const vec2& position, Ship::ship_category type,
        int hp, bool explode_on_destroy = true);

  int64_t get_score() const
  {
    return _score;
  }

  void set_score(long score)
  {
    _score = score;
  }

  int get_hp() const
  {
    return _hp;
  }

  void set_destroy_sound(Lib::Sound sound)
  {
    _destroy_sound = sound;
  }

  // Generic behaviour
  //------------------------------
  void damage(int damage, bool magic, Player* source) override;
  void render() const override;
  virtual void on_destroy(bool bomb) {}

private:

  int32_t _hp;
  int64_t _score;
  mutable int32_t _damaged;
  Lib::Sound _destroy_sound;
  bool _explode_on_destroy;

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
  void on_destroy(bool bomb) override;

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

  FollowHub(const vec2& position, bool powera = false, bool powerb = false);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  int _timer;
  vec2 _dir;
  int _count;
  bool _powera;
  bool _powerb;

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
  bool _rdir;
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
