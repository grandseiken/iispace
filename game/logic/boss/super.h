#ifndef II_GAME_LOGIC_BOSS_SUPER_H
#define II_GAME_LOGIC_BOSS_SUPER_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class SuperBossArc : public Boss {
public:
  SuperBossArc(ii::SimInterface& sim, const vec2& position, std::uint32_t players,
               std::uint32_t cycle, std::uint32_t i, Ship* boss, std::uint32_t timer = 0);

  void update() override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;
  void on_destroy() override;
  void render() const override;

  std::uint32_t GetTimer() const {
    return timer_;
  }

private:
  Ship* boss_ = nullptr;
  std::uint32_t i_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t stimer_ = 0;
};

class SuperBoss : public Boss {
public:
  enum class state { kArrive, kIdle, kAttack };

  SuperBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle);

  void update() override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;
  void on_destroy() override;

private:
  friend class SuperBossArc;
  friend class RainbowShot;

  std::uint32_t players_ = 0;
  std::uint32_t cycle_ = 0;
  std::uint32_t ctimer_ = 0;
  std::uint32_t timer_ = 0;
  std::vector<bool> destroyed_;
  std::vector<SuperBossArc*> arcs_;
  state state_ = state::kArrive;
  std::uint32_t snakes_ = 0;
};

class SnakeTail : public Enemy {
  friend class Snake;

public:
  SnakeTail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* tail_ = nullptr;
  SnakeTail* head_ = nullptr;
  std::uint32_t timer_ = 150;
  std::uint32_t dtimer_ = 0;
};

class Snake : public Enemy {
public:
  Snake(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour,
        const vec2& dir = vec2{0}, fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* tail_ = nullptr;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  std::uint32_t count_ = 0;
  glm::vec4 colour_{0.f};
  bool shot_snake_ = false;
  fixed shot_rot_ = 0;
};

class RainbowShot : public BossShot {
public:
  RainbowShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity, Ship* boss);
  void update() override;

private:
  Ship* boss_ = nullptr;
  std::uint32_t timer_ = 0;
};

#endif