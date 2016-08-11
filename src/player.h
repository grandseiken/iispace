#ifndef IISPACE_SRC_PLAYER_H
#define IISPACE_SRC_PLAYER_H

#include "ship.h"

class Player;
struct PlayerInput {
  virtual void get(const Player& player, vec2& velocity, vec2& target, int32_t& keys) = 0;
  virtual ~PlayerInput() {}
};
struct ReplayPlayerInput : PlayerInput {
  ReplayPlayerInput(const Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, int32_t& keys) override;

  const Replay& replay;
  std::size_t replay_frame;
};
struct LibPlayerInput : PlayerInput {
  LibPlayerInput(Lib& lib, Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, int32_t& keys) override;

  Lib& lib;
  Replay& replay;
};

class Player : public Ship {
public:
  static const int32_t BOMB_DAMAGE = 50;

  Player(PlayerInput& input, const vec2& position, int32_t player_number);
  ~Player() override;

  int32_t player_number() const {
    return _player_number;
  }

  void update() override;
  void render() const override;
  void damage();

  void activate_magic_shots();
  void activate_magic_shield();
  void activate_bomb();
  static void update_fire_timer();

  // Scoring
  //------------------------------
  int64_t score() const {
    return _score;
  }

  int32_t deaths() const {
    return _death_count;
  }

  void add_score(int64_t score);

  // Colour
  //------------------------------
  static colour_t player_colour(std::size_t player_number);
  colour_t colour() const {
    return player_colour(_player_number);
  }

  // Temporary death
  //------------------------------
  static std::size_t killed_players() {
    return _kill_queue.size();
  }

  bool is_killed() {
    return _kill_timer != 0;
  }

private:
  PlayerInput& _input;
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
  static GameModal::ship_list _kill_queue;
  static GameModal::ship_list _shot_sound_queue;
};

class Shot : public Ship {
public:
  Shot(const vec2& position, Player* player, const vec2& direction, bool magic = false);
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
  void damage(int32_t damage, bool magic, Player* source) override;

private:
  type_t _type;
  int32_t _frame;
  vec2 _dir;
  bool _rotate;
  bool _first_frame;
};

#endif
