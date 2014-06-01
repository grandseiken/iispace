#ifndef IISPACE_SRC_ENEMY_H
#define IISPACE_SRC_ENEMY_H

#include "ship.h"

class Enemy : public Ship {
public:

  Enemy(const vec2& position, Ship::ship_category type,
        int32_t hp, bool explode_on_destroy = true);

  int64_t get_score() const
  {
    return _score;
  }

  void set_score(long score)
  {
    _score = score;
  }

  int32_t get_hp() const
  {
    return _hp;
  }

  void set_destroy_sound(Lib::Sound sound)
  {
    _destroy_sound = sound;
  }

  void damage(int32_t damage, bool magic, Player* source) override;
  void render() const override;
  virtual void on_destroy(bool bomb) {}

private:

  int32_t _hp;
  int64_t _score;
  mutable int32_t _damaged;
  Lib::Sound _destroy_sound;
  bool _explode_on_destroy;

};

class Follow : public Enemy {
public:

  Follow(const vec2& position, fixed radius = 10, int32_t hp = 1);
  void update() override;

private:

  int32_t _timer;
  Ship* _target;

};

class Chaser : public Enemy {
public:

  Chaser(const vec2& position);
  void update() override;

private:

  bool _move;
  int32_t _timer;
  vec2 _dir;

};

class Square : public Enemy {
public:

  Square(const vec2& position, fixed rotation = fixed::pi / 2);
  void update() override;
  void render() const override;

private:

  vec2 _dir;
  int32_t _timer;

};

class Wall : public Enemy {
public:

  Wall(const vec2& position, bool rdir);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  vec2 _dir;
  int32_t  _timer;
  bool _rotate;
  bool _rdir;

};

class FollowHub : public Enemy {
public:

  FollowHub(const vec2& position, bool powera = false, bool powerb = false);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  int32_t _timer;
  vec2 _dir;
  int32_t _count;
  bool _powera;
  bool _powerb;

};

class Shielder : public Enemy {
public:

  Shielder(const vec2& position, bool power = false);
  void update() override;

private:

  vec2 _dir;
  int32_t _timer;
  bool _rotate;
  bool _rdir;
  bool _power;

};

class Tractor : public Enemy {
public:

  static const fixed TRACTOR_BEAM_SPEED;
  Tractor(const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:

  int32_t  _timer;
  vec2 _dir;
  bool _power;

  bool _ready;
  bool _spinning;
  z0Game::ShipList _players;

};

#endif
