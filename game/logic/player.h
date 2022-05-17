#ifndef IISPACE_GAME_LOGIC_PLAYER_H
#define IISPACE_GAME_LOGIC_PLAYER_H
#include "game/logic/ship.h"

class PlayerInput;

class Player : public Ship {
public:
  static constexpr std::int32_t kBombDamage = 50;

  Player(PlayerInput& input, const vec2& position, std::int32_t player_number);
  ~Player() override;

  std::int32_t player_number() const {
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
  std::int64_t score() const {
    return _score;
  }

  std::int32_t deaths() const {
    return _death_count;
  }

  void add_score(std::int64_t score);

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
  std::int32_t _player_number;
  std::int64_t _score;
  std::int32_t _multiplier;
  std::int32_t _multiplier_count;
  std::int32_t _kill_timer;
  std::int32_t _revive_timer;
  std::int32_t _magic_shot_timer;
  bool _shield;
  bool _bomb;
  vec2 _fire_target;
  std::int32_t _death_count;

  static std::int32_t _fire_timer;
  static SimState::ship_list _kill_queue;
  static SimState::ship_list _shot_sound_queue;
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
  enum class type {
    kExtraLife,
    kMagicShots,
    kShield,
    kBomb,
  };

  Powerup(const vec2& position, type t);
  void update() override;
  void damage(std::int32_t damage, bool magic, Player* source) override;

private:
  type _type;
  std::int32_t _frame;
  vec2 _dir;
  bool _rotate;
  bool _first_frame;
};

#endif
