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

  static bool _has_counted;

private:
  bool _on_screen;
  bool _move;
  std::int32_t _timer;
  vec2 _dir;
  vec2 _last_dir;

  std::int32_t _players;
  std::int32_t _cycle;
  std::int32_t _split;

  std::int32_t _stagger;
  static std::int32_t _count;
  static std::int32_t _shared_hp;
};

#endif