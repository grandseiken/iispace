#ifndef IISPACE_SRC_BOSS_CHASER_H
#define IISPACE_SRC_BOSS_CHASER_H

#include "boss.h"

class ChaserBoss : public Boss {
public:
  static const int32_t TIMER = 60;
  ChaserBoss(int32_t players, int32_t cycle, int32_t split = 0, const vec2& position = vec2(),
             int32_t time = TIMER, int32_t stagger = 0);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;
  void on_destroy() override;

  static bool _has_counted;

private:
  bool _on_screen;
  bool _move;
  int32_t _timer;
  vec2 _dir;
  vec2 _last_dir;

  int32_t _players;
  int32_t _cycle;
  int32_t _split;

  int32_t _stagger;
  static int32_t _count;
  static int32_t _shared_hp;
};

#endif