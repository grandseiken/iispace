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
    return player_number_;
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
    return score_;
  }

  std::int32_t deaths() const {
    return death_count_;
  }

  void add_score(std::int64_t score);

  // Colour
  //------------------------------
  static colour_t player_colour(std::size_t player_number);
  colour_t colour() const {
    return player_colour(player_number_);
  }

  // Temporary death
  //------------------------------
  static std::size_t killed_players() {
    return kill_queue_.size();
  }

  bool is_killed() {
    return kill_timer_ != 0;
  }

private:
  PlayerInput& input_;
  std::int32_t player_number_;
  std::int64_t score_;
  std::int32_t multiplier_;
  std::int32_t multiplier_count_;
  std::int32_t kill_timer_;
  std::int32_t revive_timer_;
  std::int32_t magic_shot_timer_;
  bool shield_;
  bool bomb_;
  vec2 fire_target_;
  std::int32_t death_count_;

  static std::int32_t fire_timer_;
  static SimState::ship_list kill_queue_;
  static SimState::ship_list shot_sound_queue_;
};

class Shot : public Ship {
public:
  Shot(const vec2& position, Player* player, const vec2& direction, bool magic = false);
  ~Shot() override {}

  void update() override;
  void render() const override;

private:
  Player* player_;
  vec2 velocity_;
  bool magic_;
  bool flash_;
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
  type type_;
  std::int32_t frame_;
  vec2 dir_;
  bool rotate_;
  bool first_frame_;
};

#endif
