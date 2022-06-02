#include "game/logic/sim/sim_interface.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship.h"
#include "game/logic/sim/sim_internals.h"

namespace ii {

colour_t SimInterface::player_colour(std::size_t player_number) {
  return player_number == 0 ? 0xff0000ff
      : player_number == 1  ? 0xff5500ff
      : player_number == 2  ? 0xffaa00ff
      : player_number == 3  ? 0xffff00ff
                            : 0x00ff00ff;
}

input_frame SimInterface::input(std::int32_t player_number) {
  if (player_number < internals_->input_frames.size()) {
    return internals_->input_frames[player_number];
  }
  return {};
}

game_mode SimInterface::mode() const {
  return internals_->mode;
}

std::int32_t SimInterface::random(std::int32_t max) {
  return internals_->random_engine() % max;
}

fixed SimInterface::random_fixed() {
  return fixed{internals_->random_engine()} / RandomEngine::rand_max;
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
  if (boss == kBoss3A || (mode() != game_mode::kBoss && mode() != game_mode::kNormal)) {
    internals_->hard_mode_bosses_killed |= boss;
  } else {
    internals_->bosses_killed |= boss;
  }
}

Ship* SimInterface::add_ship(std::unique_ptr<Ship> ship) {
  auto p = ship.get();
  if (ship->type() & Ship::kShipEnemy) {
    internals_->overmind->on_enemy_create(*ship);
  }
  if (ship->bounding_width() > 1) {
    internals_->collisions.push_back(ship.get());
  }
  internals_->ships.emplace_back(std::move(ship));
  return p;
}

void SimInterface::add_particle(const ii::particle& particle) {
  internals_->particles.emplace_back(particle);
}

void SimInterface::rumble_all(std::int32_t time) const {
  for (std::int32_t i = 0; i < players().size(); ++i) {
    rumble(i, time);
  }
}

void SimInterface::rumble(std::int32_t player, std::int32_t time) const {
  auto& rumble = internals_->rumble_output[player];
  rumble = std::max(rumble, time);
}

void SimInterface::play_sound(sound s, float volume, float pan, float repitch) const {
  auto& aggregation = internals_->sound_output[s];
  ++aggregation.count;
  aggregation.volume += volume;
  aggregation.pan += pan;
  aggregation.pitch = repitch;
}

void SimInterface::render_hp_bar(float fill) const {
  internals_->boss_hp_bar = fill;
}

void SimInterface::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
  auto& e = internals_->line_output.emplace_back();
  e.a = a;
  e.b = b;
  e.c = c;
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
  internals_->player_output.resize(
      std::max<std::size_t>(internals_->player_output.size(), player_number + 1));
  auto& info = internals_->player_output[player_number];
  info.colour = colour;
  info.score = score;
  info.multiplier = multiplier;
  info.timer = timer;
}

}  // namespace ii