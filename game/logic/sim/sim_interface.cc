#include "game/logic/sim/sim_interface.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship.h"
#include "game/logic/sim/sim_internals.h"

namespace ii {

glm::vec4 SimInterface::player_colour(std::size_t player_number) {
  return player_number == 0 ? colour_hue360(0)
      : player_number == 1  ? colour_hue360(20)
      : player_number == 2  ? colour_hue360(40)
      : player_number == 3  ? colour_hue360(60)
                            : colour_hue360(120);
}

input_frame SimInterface::input(std::uint32_t player_number) {
  if (player_number < internals_->input_frames.size()) {
    return internals_->input_frames[player_number];
  }
  return {};
}

game_mode SimInterface::mode() const {
  return internals_->mode;
}

std::uint32_t SimInterface::random(std::uint32_t max) {
  return internals_->random_engine() % max;
}

fixed SimInterface::random_fixed() {
  return fixed{static_cast<std::int32_t>(internals_->random_engine())} / RandomEngine::rand_max;
}

SimInterface::ship_list SimInterface::all_ships(std::uint32_t ship_mask) const {
  ship_list r;
  for (auto& ship : internals_->ships) {
    if (!ship_mask || (ship->type() & ship_mask)) {
      r.push_back(ship.get());
    }
  }
  return r;
}

SimInterface::ship_list
SimInterface::ships_in_radius(const vec2& point, fixed radius, std::uint32_t ship_mask) const {
  ship_list r;
  for (auto& ship : internals_->ships) {
    if ((!ship_mask || (ship->type() & ship_mask)) &&
        length(ship->shape().centre - point) <= radius) {
      r.push_back(ship.get());
    }
  }
  return r;
}

bool SimInterface::any_collision(const vec2& point, std::uint32_t category) const {
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
SimInterface::collision_list(const vec2& point, std::uint32_t category) const {
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

std::uint32_t SimInterface::get_non_wall_count() const {
  return internals_->overmind->count_non_wall_enemies();
}

std::uint32_t SimInterface::player_count() const {
  return static_cast<std::uint32_t>(players().size());
}

std::uint32_t SimInterface::alive_players() const {
  return player_count() - killed_players();
}

std::uint32_t SimInterface::killed_players() const {
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
    auto d = length_squared(s->shape().centre - point);
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

std::uint32_t SimInterface::get_lives() const {
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

void SimInterface::rumble_all(std::uint32_t time) const {
  for (std::uint32_t i = 0; i < player_count(); ++i) {
    rumble(i, time);
  }
}

void SimInterface::rumble(std::uint32_t player, std::uint32_t time) const {
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

void SimInterface::render_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) const {
  auto& e = internals_->line_output.emplace_back();
  e.a = a;
  e.b = b;
  e.c = c;
}

void SimInterface::render_line_rect(const glm::vec2& lo, const glm::vec2& hi,
                                    const glm::vec4& c) const {
  glm::vec2 li{lo.x, hi.y};
  glm::vec2 ho{hi.x, lo.y};
  render_line(lo, li, c);
  render_line(li, hi, c);
  render_line(hi, ho, c);
  render_line(ho, lo, c);
}

void SimInterface::render_player_info(std::uint32_t player_number, const glm::vec4& colour,
                                      std::uint64_t score, std::uint32_t multiplier, float timer) {
  internals_->player_output.resize(
      std::max<std::size_t>(internals_->player_output.size(), player_number + 1));
  auto& info = internals_->player_output[player_number];
  info.colour = colour;
  info.score = score;
  info.multiplier = multiplier;
  info.timer = timer;
}

}  // namespace ii