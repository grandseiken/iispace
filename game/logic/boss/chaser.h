#ifndef IISPACE_GAME_LOGIC_BOSS_CHASER_H
#define IISPACE_GAME_LOGIC_BOSS_CHASER_H
#include "game/logic/boss.h"

class ChaserBoss : public Boss {
public:
  static constexpr std::int32_t kTimer = 60;
  ChaserBoss(std::int32_t players, std::int32_t cycle, std::int32_t split = 0,
             const vec2& position = vec2(), std::int32_t time = kTimer, std::int32_t stagger = 0);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;
  void on_destroy() override;

  static bool has_counted_;

private:
  bool on_screen_;
  bool move_;
  std::int32_t timer_;
  vec2 dir_;
  vec2 last_dir_;

  std::int32_t players_;
  std::int32_t cycle_;
  std::int32_t split_;

  std::int32_t stagger_;
  static std::int32_t count_;
  static std::int32_t shared_hp_;
};

#endif