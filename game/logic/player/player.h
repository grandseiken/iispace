#ifndef II_GAME_LOGIC_PLAYER_PLAYER_H
#define II_GAME_LOGIC_PLAYER_PLAYER_H
#include "game/logic/ecs/id.h"
#include "game/logic/ship/ship.h"

class Player;

namespace ii {
struct Player : ecs::component {
  std::uint32_t player_number = 0;
  std::uint32_t kill_timer = 0;

  std::uint64_t score = 0;
  std::uint32_t multiplier = 1;
  std::uint32_t multiplier_count = 0;
  std::uint32_t death_count = 0;

  bool is_killed() const {
    return kill_timer != 0;
  }
  void add_score(SimInterface&, std::uint64_t s);
};

enum class powerup_type {
  kExtraLife,
  kMagicShots,
  kShield,
  kBomb,
};

void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number);
void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);

}  // namespace ii

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
    return handle().get<ii::Player>()->is_killed();
  }

private:
  std::uint32_t player_number_ = 0;
  std::uint32_t revive_timer_ = 0;
  std::uint32_t magic_shot_timer_ = 0;
  bool shield_ = false;
  bool bomb_ = false;
  vec2 fire_target_;

  static std::uint32_t fire_timer_;
  static ii::SimInterface::ship_list kill_queue_;
};

#endif
