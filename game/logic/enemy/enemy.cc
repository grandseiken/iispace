#include "game/logic/enemy/enemy.h"

namespace ii {

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const glm::vec4& c) {
  auto h = sim.create_legacy(std::make_unique<BossShot>(sim, position, velocity, c));
  h.add(legacy_collision(/* bounding width */ 12));
  h.add(Enemy{.threat_value = 1});
  h.add(Health{.hp = 0, .on_destroy = &legacy_enemy_on_destroy});
}

}  // namespace ii

Enemy::Enemy(ii::SimInterface& sim, const vec2& position, ii::ship_flag type)
: ii::Ship{sim, position, type | ii::ship_flag::kEnemy} {}

void Enemy::render() const {
  if (auto c = handle().get<ii::Health>(); c && c->hit_timer) {
    for (std::size_t i = 0; i < shapes().size(); ++i) {
      bool hit_flash = !c->hit_flash_ignore_index || i < *c->hit_flash_ignore_index;
      shapes()[i]->render(sim(), to_float(shape().centre), shape().rotation().to_float(),
                          hit_flash ? std::make_optional(glm::vec4{1.f}) : std::nullopt);
    }
    return;
  }
  Ship::render();
}

BossShot::BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                   const glm::vec4& c)
: Enemy{sim, position, ii::ship_flag::kWall}, dir_{velocity} {
  add_new_shape<ii::Polygon>(vec2{0}, 16, 8, c, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 8, c, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 12, 8, glm::vec4{0.f}, 0, ii::shape_flag::kDangerous);
}

void BossShot::update() {
  move(dir_);
  vec2 p = shape().centre;
  if ((p.x < -10 && dir_.x < 0) || (p.x > ii::kSimDimensions.x + 10 && dir_.x > 0) ||
      (p.y < -10 && dir_.y < 0) || (p.y > ii::kSimDimensions.y + 10 && dir_.y > 0)) {
    destroy();
  }
  shape().set_rotation(shape().rotation() + fixed_c::hundredth * 2);
}