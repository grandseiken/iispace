#include "game/logic/sim_interface.h"
#include "game/core/lib.h"
#include "game/core/z0_game.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship.h"
#include "game/logic/sim_internals.h"
#include <sstream>

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
  if (boss == BOSS_3A || (mode() != game_mode::kBoss && mode() != game_mode::kNormal)) {
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

void SimInterface::rumble(std::int32_t player, std::int32_t time) const {
  lib_.rumble(player, time);
}

void SimInterface::play_sound(sound s, float volume, float pan, float repitch) const {
  lib_.play_sound(s, volume, pan, repitch);
}

void SimInterface::render_hp_bar(float fill) const {
  internals_->boss_hp_bar = fill;
}

void SimInterface::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
  lib_.render_line(a, b, c);
}

void SimInterface::render_line_rect(const fvec2& lo, const fvec2& hi, colour_t c) const {
  fvec2 li{lo.x, hi.y};
  fvec2 ho{hi.x, lo.y};
  render_line(lo, li, c);
  render_line(li, hi, c);
  render_line(hi, ho, c);
  render_line(ho, lo, c);
}

void SimInterface::render_player_info(std::int32_t player_number, colour_t colour,
                                      std::int64_t score, std::int32_t multiplier, float timer) {
  std::stringstream ss;
  ss << multiplier << "X";
  std::string s = ss.str();
  std::int32_t n = player_number;
  fvec2 v = n == 1 ? fvec2{ii::kSimWidth / 16 - 1.f - s.length(), 1.f}
      : n == 2     ? fvec2{1.f, ii::kSimHeight / 16 - 2.f}
      : n == 3     ? fvec2{ii::kSimWidth / 16 - 1.f - s.length(), ii::kSimHeight / 16 - 2.f}
                   : fvec2{1.f, 1.f};
  lib_.render_text(v, s, z0Game::kPanelText);

  ss.str("");
  n % 2 ? ss << score << "   " : ss << "   " << score;
  lib_.render_text(
      v - (n % 2 ? fvec2{static_cast<float>(ss.str().length() - s.length()), 0} : fvec2{}),
      ss.str(), colour);

  if (timer) {
    v.x += n % 2 ? -1 : ss.str().length();
    v *= 16;
    render_line_rect(v + fvec2{5.f, 11.f - 10 * timer}, v + fvec2{9.f, 13.f}, 0xffffffff);
  }
}

}  // namespace ii