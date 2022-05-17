#ifndef IISPACE_GAME_LOGIC_ENEMY_H
#define IISPACE_GAME_LOGIC_ENEMY_H
#include "game/logic/ship.h"

class Enemy : public Ship {
public:
  Enemy(const vec2& position, Ship::ship_category type, std::int32_t hp);

  std::int64_t get_score() const {
    return _score;
  }

  void set_score(long score) {
    _score = score;
  }

  std::int32_t get_hp() const {
    return _hp;
  }

  void set_destroy_sound(Lib::sound sound) {
    _destroy_sound = sound;
  }

  void damage(std::int32_t damage, bool magic, Player* source) override;
  void render() const override;
  virtual void on_destroy(bool bomb) {}

private:
  std::int32_t _hp;
  std::int64_t _score;
  mutable std::int32_t _damaged;
  Lib::sound _destroy_sound;
};

class Follow : public Enemy {
public:
  Follow(const vec2& position, fixed radius = 10, std::int32_t hp = 1);
  void update() override;

private:
  std::int32_t _timer;
  Ship* _target;
};

class BigFollow : public Follow {
public:
  BigFollow(const vec2& position, bool has_score);
  void on_destroy(bool bomb) override;

private:
  bool _has_score;
};

class Chaser : public Enemy {
public:
  Chaser(const vec2& position);
  void update() override;

private:
  bool _move;
  std::int32_t _timer;
  vec2 _dir;
};

class Square : public Enemy {
public:
  Square(const vec2& position, fixed rotation = fixed_c::pi / 2);
  void update() override;
  void render() const override;

private:
  vec2 _dir;
  std::int32_t _timer;
};

class Wall : public Enemy {
public:
  Wall(const vec2& position, bool rdir);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  vec2 _dir;
  std::int32_t _timer;
  bool _rotate;
  bool _rdir;
};

class FollowHub : public Enemy {
public:
  FollowHub(const vec2& position, bool powera = false, bool powerb = false);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  std::int32_t _timer;
  vec2 _dir;
  std::int32_t _count;
  bool _powera;
  bool _powerb;
};

class Shielder : public Enemy {
public:
  Shielder(const vec2& position, bool power = false);
  void update() override;

private:
  vec2 _dir;
  std::int32_t _timer;
  bool _rotate;
  bool _rdir;
  bool _power;
};

class Tractor : public Enemy {
public:
  static const fixed kTractorBeamSpeed;
  Tractor(const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:
  std::int32_t _timer;
  vec2 _dir;
  bool _power;

  bool _ready;
  bool _spinning;
  SimState::ship_list _players;
};

class BossShot : public Enemy {
public:
  BossShot(const vec2& position, const vec2& velocity, colour_t c = 0x999999ff);
  void update() override;

protected:
  vec2 _dir;
};

#endif
