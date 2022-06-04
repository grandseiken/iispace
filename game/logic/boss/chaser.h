#ifndef II_GAME_LOGIC_BOSS_CHASER_H
#define II_GAME_LOGIC_BOSS_CHASER_H
#include "game/logic/boss.h"

class ChaserBoss : public Boss {
public:
  static constexpr std::uint32_t kTimer = 60;
  ChaserBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle,
             std::uint32_t split = 0, const vec2& position = vec2{0}, std::uint32_t time = kTimer,
             std::uint32_t stagger = 0);

  void update() override;
  void render() const override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;
  void on_destroy() override;

  static bool has_counted_;

private:
  bool on_screen_ = false;
  bool move_ = false;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  vec2 last_dir_{0};

  std::uint32_t players_ = 0;
  std::uint32_t cycle_ = 0;
  std::uint32_t split_ = 0;

  std::uint32_t stagger_ = 0;
  static std::uint32_t count_;
  static std::uint32_t shared_hp_;
};

#endif