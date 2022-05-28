#include "game/logic/sim_interface.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship.h"
#include "game/logic/sim_internals.h"

namespace ii {

game_mode SimInterface::mode() const {
  return internals_->mode;
}

SimInterface::ship_list SimInterface::all_ships(std::int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : internals_->ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

SimInterface::ship_list
SimInterface::ships_in_radius(const vec2& point, fixed radius, std::int32_t ship_mask) const {
  ship_list r;
  for (auto& ship : internals_->ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        (ship->shape().centre - point).length() <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool SimInterface::any_collision(const vec2& point, std::int32_t category) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    fixed sx = collision->shape().centre.x;
    fixed sy = collision->shape().centre.y;
    fixed w = collision->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (collision->check_point(point, category)) {
      return true;
    }
  }
  return false;
}

SimInterface::ship_list
SimInterface::collision_list(const vec2& point, std::int32_t category) const {
  ship_list r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    fixed sx = collision->shape().centre.x;
    fixed sy = collision->shape().centre.y;
    fixed w = collision->bounding_width();

    if (sx - w > x) {
      break;
    }
    if (sx + w < x || sy + w < y || sy - w > y) {
      continue;
    }

    if (collision->check_point(point, category)) {
      r.push_back(collision);
    }
  }
  return r;
}

std::int32_t SimInterface::get_non_wall_count() const {
  return internals_->overmind->count_non_wall_enemies();
}

std::int32_t SimInterface::alive_players() const {
  return players().size() - killed_players();
}

std::int32_t SimInterface::killed_players() const {
  return Player::killed_players();
}

const SimInterface::ship_list& SimInterface::players() const {
  return internals_->player_list;
}

Player* SimInterface::nearest_player(const vec2& point) const {
  Ship* ship = nullptr;
  Ship* dead = nullptr;
  fixed ship_dist = 0;
  fixed dead_dist = 0;

  for (Ship* s : internals_->player_list) {
    fixed d = (s->shape().centre - point).length_squared();
    if ((d < ship_dist || !ship) && !((Player*)s)->is_killed()) {
      ship_dist = d;
      ship = s;
    }
    if (d < dead_dist || !dead) {
      dead_dist = d;
      dead = s;
    }
  }
  return (Player*)(ship ? ship : dead);
}

void SimInterface::add_life() {
  ++internals_->lives;
}

void SimInterface::sub_life() {
  if (internals_->lives) {
    internals_->lives--;
  }
}

std::int32_t SimInterface::get_lives() const {
  return internals_->lives;
}

void SimInterface::set_boss_killed(boss_list boss) {
  if (boss == ii::SimInterface::BOSS_3A ||
      (mode() != game_mode::kBoss && mode() != game_mode::kNormal)) {
    internals_->hard_mode_bosses_killed |= boss;
  } else {
    internals_->bosses_killed |= boss;
  }
}

void SimInterface::add_ship(std::unique_ptr<Ship> ship) {
  ship->set_sim(*this);
  if (ship->type() & Ship::kShipEnemy) {
    internals_->overmind->on_enemy_create(*ship);
  }
  if (ship->bounding_width() > 1) {
    internals_->collisions.push_back(ship.get());
  }
  internals_->ships.emplace_back(std::move(ship));
}

void SimInterface::add_particle(const Particle& particle) {
  internals_->particles.emplace_back(particle);
}

void SimInterface::render_hp_bar(float fill) {
  internals_->boss_hp_bar = fill;
}

}  // namespace ii