#ifndef II_GAME_LOGIC_ENEMY_H
#define II_GAME_LOGIC_ENEMY_H
#include "game/logic/ship/ship.h"

class Enemy : public ii::Ship {
public:
  Enemy(ii::SimInterface& sim, const vec2& position, Ship::ship_category type, std::uint32_t hp);

  std::uint64_t get_score() const {
    return score_;
  }

  void set_score(long score) {
    score_ = score;
  }

  std::uint32_t get_hp() const {
    return hp_;
  }

  void set_destroy_sound(ii::sound sound) {
    destroy_sound_ = sound;
  }

  void damage(std::uint32_t damage, bool magic, Player* source) override;
  void render() const override;
  virtual void on_destroy(bool bomb) {}

private:
  std::uint32_t hp_ = 0;
  std::uint64_t score_ = 0;
  mutable std::uint32_t damaged_ = 0;
  ii::sound destroy_sound_ = ii::sound::kEnemyDestroy;
};

class Follow : public Enemy {
public:
  Follow(ii::SimInterface& sim, const vec2& position, fixed radius = 10, std::uint32_t hp = 1);
  void update() override;

private:
  std::uint32_t timer_ = 0;
  Ship* target_ = nullptr;
};

class BigFollow : public Follow {
public:
  BigFollow(ii::SimInterface& sim, const vec2& position, bool has_score);
  void on_destroy(bool bomb) override;

private:
  bool has_score_ = 0;
};

class Chaser : public Enemy {
public:
  Chaser(ii::SimInterface& sim, const vec2& position);
  void update() override;

private:
  bool move_ = false;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
};

class Square : public Enemy {
public:
  Square(ii::SimInterface& sim, const vec2& position, fixed rotation = fixed_c::pi / 2);
  void update() override;
  void render() const override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
};

class Wall : public Enemy {
public:
  Wall(ii::SimInterface& sim, const vec2& position, bool rdir);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
  bool rotate_ = false;
  bool rdir_ = false;
};

class FollowHub : public Enemy {
public:
  FollowHub(ii::SimInterface& sim, const vec2& position, bool powera = false, bool powerb = false);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  std::uint32_t count_ = 0;
  bool powera_ = false;
  bool powerb_ = false;
};

class Shielder : public Enemy {
public:
  Shielder(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
  bool rotate_ = false;
  bool rdir_ = false;
  bool power_ = false;
};

class Tractor : public Enemy {
public:
  static const fixed kTractorBeamSpeed;
  Tractor(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  bool power_ = false;

  bool ready_ = false;
  bool spinning_ = false;
  ii::SimInterface::ship_list players_;
};

class BossShot : public Enemy {
public:
  BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
           const glm::vec4& c = {0.f, 0.f, .6f, 1.f});
  void update() override;

protected:
  vec2 dir_{0};
};

#endif
