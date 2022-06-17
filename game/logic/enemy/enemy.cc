#include "game/logic/enemy/enemy.h"

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