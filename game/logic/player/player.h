#ifndef II_GAME_LOGIC_PLAYER_PLAYER_H
#define II_GAME_LOGIC_PLAYER_PLAYER_H
#include "game/logic/ecs/id.h"
#include "game/logic/ship/components.h"
#include "game/logic/ship/ship.h"

class Player;

namespace ii {
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
  ~Player() override = default;

  void update() override;
  void render() const override;
  void damage();

  void activate_magic_shield();
  void activate_bomb();

  // Temporary death
  //------------------------------
  static std::uint32_t killed_players() {
    return static_cast<std::uint32_t>(kill_queue_.size());
  }

  vec2& position() override {
    return handle().get<ii::Transform>()->centre;
  }
  vec2 position() const override {
    return handle().get<ii::Transform>()->centre;
  }
  fixed rotation() const override {
    return handle().get<ii::Transform>()->rotation;
  }

private:
  static ii::SimInterface::ship_list kill_queue_;
};

#endif
