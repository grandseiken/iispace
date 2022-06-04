#ifndef II_GAME_LOGIC_PLAYER_H
#define II_GAME_LOGIC_PLAYER_H
#include "game/logic/ship/ship.h"

class Player : public ii::Ship {
public:
  static constexpr std::uint32_t kBombDamage = 50;

  Player(ii::SimInterface& sim, const vec2& position, std::uint32_t player_number);
  ~Player() override;

  std::uint32_t player_number() const {
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
  std::uint64_t score() const {
    return score_;
  }

  std::uint32_t deaths() const {
    return death_count_;
  }

  void add_score(std::uint64_t score);

  // Colour
  //------------------------------
  glm::vec4 colour() const {
    return ii::SimInterface::player_colour(player_number_);
  }

  // Temporary death
  //------------------------------
  static std::uint32_t killed_players() {
    return static_cast<std::uint32_t>(kill_queue_.size());
  }

  bool is_killed() {
    return kill_timer_ != 0;
  }

private:
  std::uint32_t player_number_ = 0;
  std::uint64_t score_ = 0;
  std::uint32_t multiplier_ = 1;
  std::uint32_t multiplier_count_ = 0;
  std::uint32_t kill_timer_ = 0;
  std::uint32_t revive_timer_ = 0;
  std::uint32_t magic_shot_timer_ = 0;
  bool shield_ = false;
  bool bomb_ = false;
  vec2 fire_target_;
  std::uint32_t death_count_ = 0;

  static std::uint32_t fire_timer_;
  static ii::SimInterface::ship_list kill_queue_;
};

class Shot : public ii::Ship {
public:
  Shot(ii::SimInterface& sim, const vec2& position, Player* player, const vec2& direction,
       bool magic = false);
  ~Shot() override {}

  void update() override;
  void render() const override;

private:
  Player* player_ = nullptr;
  vec2 velocity_;
  bool magic_ = false;
  bool flash_ = false;
};

class Powerup : public ii::Ship {
public:
  enum class type {
    kExtraLife,
    kMagicShots,
    kShield,
    kBomb,
  };

  Powerup(ii::SimInterface& sim, const vec2& position, type t);
  void update() override;
  void damage(std::uint32_t damage, bool magic, Player* source) override;

private:
  type type_ = type::kExtraLife;
  std::uint32_t frame_ = 0;
  vec2 dir_ = {0, 1};
  bool rotate_ = false;
  bool first_frame_ = true;
};

#endif
