#ifndef IISPACE_SRC_PLAYER_H
#define IISPACE_SRC_PLAYER_H

#include "ship.h"
#include "replay.h"

class Player : public Ship {
public:

  static const int32_t bomb_damage = 50;

  // Player ship
  //------------------------------
  Player(const vec2& position, int32_t player_number);
  ~Player() override;

  int32_t player_number() const
  {
    return _player_number;
  }

  static Replay replay;
  static std::size_t replay_frame;

  // Player behaviour
  //------------------------------
  void update() override;
  void render() const override;
  void damage();

  void activate_magic_shots();
  void activate_magic_shield();
  void activate_bomb();
  static void update_fire_timer();

  // Scoring
  //------------------------------
  int64_t score() const
  {
    return _score;
  }

  int32_t deaths() const
  {
    return _death_count;
  }

  void add_score(int64_t score);

  // Colour
  //------------------------------
  static colour_t player_colour(std::size_t player_number);
  colour_t colour() const
  {
    return player_colour(_player_number);
  }

  // Temporary death
  //------------------------------
  static std::size_t killed_players()
  {
    return _kill_queue.size();
  }

  bool is_killed()
  {
    return _kill_timer != 0;
  }

private:

  // Internals
  //------------------------------
  int32_t _player_number;
  int64_t _score;
  int32_t _multiplier;
  int32_t _multiplier_count;
  int32_t _kill_timer;
  int32_t _revive_timer;
  int32_t _magic_shot_timer;
  bool _shield;
  bool _bomb;
  vec2 _fire_target;
  int32_t _death_count;

  static int32_t _fire_timer;
  static z0Game::ShipList _kill_queue;
  static z0Game::ShipList _shot_sound_queue;

};

class Shot : public Ship {
public:

  Shot(const vec2& position, Player* player,
       const vec2& direction, bool magic = false);
  ~Shot() override {}

  void update() override;
  void render() const override;

private:

  Player* _player;
  vec2 _velocity;
  bool _magic;
  bool _flash;

};

class Powerup : public Ship {
public:

  enum type_t {
    EXTRA_LIFE,
    MAGIC_SHOTS,
    SHIELD,
    BOMB,
  };

  Powerup(const vec2& position, type_t type);
  void update() override;
  void damage(int damage, bool magic, Player* source) override;

private:

  type_t _type;
  int32_t _frame;
  vec2 _dir;
  bool _rotate;
  bool _first_frame;

};

#endif
