#include "game/logic/sim/sim_interface.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship/ship.h"
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

const initial_conditions& SimInterface::conditions() const {
  return internals_->conditions;
}

ecs::EntityIndex& SimInterface::index() {
  return internals_->index;
}

const ecs::EntityIndex& SimInterface::index() const {
  return internals_->index;
}

std::uint32_t SimInterface::random(std::uint32_t max) {
  return internals_->random_engine() % max;
}

fixed SimInterface::random_fixed() {
  return fixed{static_cast<std::int32_t>(internals_->random_engine())} / RandomEngine::rand_max;
}

std::size_t SimInterface::count_ships(ship_flag mask, ship_flag exclude_mask) const {
  std::size_t r = 0;
  internals_->index.iterate<LegacyShip>([&r, mask, exclude_mask](auto& c) {
    auto t = c.ship->type();
    r += (!mask || +(c.ship->type() & mask)) && !(exclude_mask & t);
  });
  return r;
}

SimInterface::ship_list SimInterface::all_ships(ship_flag mask) const {
  ship_list r;
  internals_->index.iterate<LegacyShip>([&r, mask](auto& c) {
    if (!mask || +(c.ship->type() & mask)) {
      r.emplace_back(c.ship.get());
    }
  });
  return r;
}

SimInterface::ship_list
SimInterface::ships_in_radius(const vec2& point, fixed radius, ship_flag mask) const {
  ship_list r;
  internals_->index.iterate<LegacyShip>([&r, &point, radius, mask](auto& c) {
    if ((!mask || +(c.ship->type() & mask)) && length(c.ship->shape().centre - point) <= radius) {
      r.emplace_back(c.ship.get());
    }
  });
  return r;
}

bool SimInterface::any_collision(const vec2& point, shape_flag category) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    auto& e = *collision.handle.get<Collision>();
    auto v = e.centre();
    fixed w = collision.bounding_width;

    // TODO: this optmization check is incorrect, since collision list is sorted based on x-min
    // at start of frame; may have moved in the meantime, or list may have been appended to.
    // May be impossible to fix without breaking replay compatibility, unless there is a clever
    // way to work around it with update ordering or something.
    // Very unclear what effective collision semantics actually are.
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }

    if (e.check(point, category)) {
      return true;
    }
  }
  return false;
}

SimInterface::ship_list SimInterface::collision_list(const vec2& point, shape_flag category) const {
  ship_list r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    auto& e = *collision.handle.get<Collision>();
    auto v = e.centre();
    fixed w = collision.bounding_width;

    // TODO: same as above.
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }

    if (e.check(point, category)) {
      if (auto s = collision.handle.get<LegacyShip>(); s) {
        r.push_back(s->ship.get());
      }
    }
  }
  return r;
}

std::uint32_t SimInterface::get_non_wall_count() const {
  return internals_->non_wall_enemy_count;
}

std::uint32_t SimInterface::player_count() const {
  return static_cast<std::uint32_t>(players().size());
}

std::uint32_t SimInterface::alive_players() const {
  return player_count() - killed_players();
}

std::uint32_t SimInterface::killed_players() const {
  return ::Player::killed_players();
}

SimInterface::ship_list SimInterface::players() const {
  ship_list r;
  index().iterate<Player>([&](ecs::const_handle h, const Player&) {
    if (auto c = h.get<LegacyShip>(); c) {
      r.emplace_back(c->ship.get());
    }
  });
  return r;
}

::Player* SimInterface::nearest_player(const vec2& point) const {
  Ship* ship = nullptr;
  Ship* dead = nullptr;
  fixed ship_dist = 0;
  fixed dead_dist = 0;

  for (Ship* s : players()) {
    auto d = length_squared(s->shape().centre - point);
    if ((d < ship_dist || !ship) && !((::Player*)s)->is_killed()) {
      ship_dist = d;
      ship = s;
    }
    if (d < dead_dist || !dead) {
      dead_dist = d;
      dead = s;
    }
  }
  return (::Player*)(ship ? ship : dead);
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
  if (boss == kBoss3A ||
      (internals_->conditions.mode != game_mode::kBoss &&
       internals_->conditions.mode != game_mode::kNormal)) {
    internals_->hard_mode_bosses_killed |= boss;
  } else {
    internals_->bosses_killed |= boss;
  }
}

ecs::handle SimInterface::create_legacy(std::unique_ptr<Ship> ship) {
  auto p = ship.get();
  auto h = internals_->index.create();
  h.emplace<LegacyShip>().ship = std::move(ship);
  h.emplace<Update>().update = [p] { p->update(); };
  h.emplace<Render>().render = [p] { p->render(); };
  p->set_handle(h);
  return h;
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