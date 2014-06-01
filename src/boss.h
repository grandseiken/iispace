#ifndef IISPACE_SRC_BOSS_H
#define IISPACE_SRC_BOSS_H

#include "ship.h"

class Boss : public Ship {
public:

  Boss(const vec2& position, z0Game::boss_list boss,
       int32_t hp, int32_t players, int32_t cycle = 0,
       bool explode_on_damage = true);

  void set_killed()
  {
    z0().set_boss_killed(_flag);
  }

  int64_t get_score()
  {
    return _score;
  }

  int32_t get_remaining_hp() const
  {
    return _hp / 30;
  }

  int32_t get_max_hp() const
  {
    return _max_hp / 30;
  }

  void restore_hp(int32_t hp)
  {
    _hp += hp;
  }

  bool is_hp_low() const
  {
    return (_max_hp * 1.2f) / _hp >= 3;
  }

  void set_ignore_damage_colour_index(int32_t shape_index)
  {
    _ignore_damage_colour = shape_index;
  }

  void render_hp_bar() const;

  void damage(int32_t damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hp_bar) const;
  virtual int32_t get_damage(int32_t damage, bool magic) = 0;
  virtual void on_destroy();

  static std::vector<std::pair<int32_t, std::pair<vec2, colour_t>>> _fireworks;
  static std::vector<vec2> _warnings;

protected:

  static int32_t CalculateHP(int32_t base, int32_t players, int32_t cycle);

private:

  int32_t _hp;
  int32_t _max_hp;
  z0Game::boss_list _flag;
  int64_t _score;
  int32_t _ignore_damage_colour;
  mutable int32_t _damaged;
  mutable bool _show_hp;
  bool _explode_on_damage;

};

class BigSquareBoss : public Boss {
public:

  BigSquareBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:

  vec2 _dir;
  bool _reverse;
  int32_t _timer;
  int32_t _spawn_timer;
  int32_t _special_timer;
  bool _special_attack;
  bool _special_attack_rotate;
  Player* _attack_player;

};

class ShieldBombBoss : public Boss {
public:

  ShieldBombBoss(int32_t players, int32_t cycle);

  void update() override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:

  int32_t _timer;
  int32_t _count;
  int32_t _unshielded;
  int32_t _attack;
  bool _side;
  vec2 _attack_dir;
  bool _shot_alternate;

};

class ChaserBoss : public Boss {
public:

  static const int32_t TIMER = 60;
  ChaserBoss(int32_t players, int32_t cycle, int32_t split = 0,
             const vec2& position = vec2(),
             int32_t time = TIMER, int32_t stagger = 0);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;
  void on_destroy() override;

  static bool _has_counted;

private:

  bool _on_screen;
  bool _move;
  int32_t _timer;
  vec2 _dir;
  vec2 _last_dir;

  int32_t _players;
  int32_t _cycle;
  int32_t _split;

  int32_t _stagger;
  static int32_t _count;
  static int32_t _shared_hp;

};

class TractorBoss : public Boss {
public:

  TractorBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:

  CompoundShape* _s1;
  CompoundShape* _s2;
  Polygon* _sattack;
  bool _will_attack;
  bool _stopped;
  bool _generating;
  bool _attacking;
  bool _continue;
  bool _gen_dir;
  int32_t _shoot_type;
  bool _sound;
  int32_t _timer;
  int32_t _attack_size;
  std::size_t _attack_shapes;

  std::vector<vec2> _targets;

};

class GhostBoss : public Boss {
public:

  GhostBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:

  bool _visible;
  int32_t _vtime;
  int32_t _timer;
  int32_t _attack_time;
  int32_t _attack;
  bool _rDir;
  int32_t _start_time;
  int32_t _danger_circle;
  int32_t _danger_offset1;
  int32_t _danger_offset2;
  int32_t _danger_offset3;
  int32_t _danger_offset4;
  bool _shot_type;
};

class DeathRayBoss : public Boss {
public:

  static const int32_t ARM_RTIMER = 400;
  DeathRayBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

  void on_arm_death(Ship* arm);

private:

  int32_t _timer;
  bool _laser;
  bool _dir;
  int32_t _pos;
  z0Game::ShipList _arms;
  int32_t _arm_timer;
  int32_t _shot_timer;

  int32_t _ray_attack_timer;
  vec2 _ray_src1;
  vec2 _ray_src2;
  vec2 _ray_dest;

  std::vector<std::pair<int32_t, int32_t>> _shot_queue;

};

class SuperBossArc : public Boss {
public:

  SuperBossArc(const vec2& position, int32_t players, int32_t cycle,
               int32_t i, Ship* boss, int32_t timer = 0);

  void update() override;
  int32_t get_damage(int32_t damage, bool magic) override;
  void on_destroy() override;
  void render() const override;

  int32_t GetTimer() const
  {
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

  enum State {
    STATE_ARRIVE,
    STATE_IDLE,
    STATE_ATTACK
  };

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

#endif
