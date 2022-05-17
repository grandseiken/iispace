#ifndef IISPACE_GAME_LOGIC_BOSS_SUPER_H
#define IISPACE_GAME_LOGIC_BOSS_SUPER_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class SuperBossArc : public Boss {
public:
  SuperBossArc(const vec2& position, std::int32_t players, std::int32_t cycle, std::int32_t i,
               Ship* boss, std::int32_t timer = 0);

  void update() override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;
  void on_destroy() override;
  void render() const override;

  std::int32_t GetTimer() const {
    return _timer;
  }

private:
  Ship* _boss;
  std::int32_t _i;
  std::int32_t _timer;
  std::int32_t _stimer;
};

class SuperBoss : public Boss {
public:
  enum class state { kArrive, kIdle, kAttack };

  SuperBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;
  void on_destroy() override;

private:
  friend class SuperBossArc;
  friend class RainbowShot;

  std::int32_t _players;
  std::int32_t _cycle;
  std::int32_t _ctimer;
  std::int32_t _timer;
  std::vector<bool> _destroyed;
  std::vector<SuperBossArc*> _arcs;
  state _state;
  std::int32_t _snakes;
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
  std::int32_t _timer;
  std::int32_t _dtimer;
};

class Snake : public Enemy {
public:
  Snake(const vec2& position, colour_t colour, const vec2& dir = vec2(), fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* _tail;
  std::int32_t _timer;
  vec2 _dir;
  std::int32_t _count;
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
  std::int32_t _timer;
};

#endif