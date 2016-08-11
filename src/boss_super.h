#ifndef IISPACE_SRC_BOSS_SUPER_H
#define IISPACE_SRC_BOSS_SUPER_H

#include "boss.h"
#include "enemy.h"

class SuperBossArc : public Boss {
public:
  SuperBossArc(const vec2& position, int32_t players, int32_t cycle, int32_t i, Ship* boss,
               int32_t timer = 0);

  void update() override;
  int32_t get_damage(int32_t damage, bool magic) override;
  void on_destroy() override;
  void render() const override;

  int32_t GetTimer() const {
    return _timer;
  }

private:
  Ship* _boss;
  int32_t _i;
  int32_t _timer;
  int32_t _stimer;
};

class SuperBoss : public Boss {
public:
  enum State { STATE_ARRIVE, STATE_IDLE, STATE_ATTACK };

  SuperBoss(int32_t players, int32_t cycle);

  void update() override;
  int32_t get_damage(int32_t damage, bool magic) override;
  void on_destroy() override;

private:
  friend class SuperBossArc;
  friend class RainbowShot;

  int32_t _players;
  int32_t _cycle;
  int32_t _ctimer;
  int32_t _timer;
  std::vector<bool> _destroyed;
  std::vector<SuperBossArc*> _arcs;
  State _state;
  int32_t _snakes;
};

class SnakeTail : public Enemy {
  friend class Snake;

public:
  SnakeTail(const vec2& position, colour_t colour);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* _tail;
  SnakeTail* _head;
  int32_t _timer;
  int32_t _dtimer;
};

class Snake : public Enemy {
public:
  Snake(const vec2& position, colour_t colour, const vec2& dir = vec2(), fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* _tail;
  int32_t _timer;
  vec2 _dir;
  int32_t _count;
  colour_t _colour;
  bool _shot_snake;
  fixed _shot_rot;
};

class RainbowShot : public BossShot {
public:
  RainbowShot(const vec2& position, const vec2& velocity, Ship* boss);
  void update() override;

private:
  Ship* _boss;
  int32_t _timer;
};

#endif