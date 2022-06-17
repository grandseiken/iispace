#include "game/logic/sim/sim_interface.h"
#include "game/logic/ecs/call.h"
#include "game/logic/overmind.h"
#include "game/logic/player.h"
#include "game/logic/ship/ship.h"
#include "game/logic/sim/sim_internals.h"
#include <glm/gtc/constants.hpp>

namespace ii {
namespace {
vec2 get_centre(const Transform* transform, const LegacyShip* legacy_ship) {
  if (transform) {
    return transform->centre;
  }
  return legacy_ship->ship->position();
}
}  // namespace

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

std::uint64_t SimInterface::tick_count() const {
  return internals_->tick_count;
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
    if ((!mask || +(c.ship->type() & mask)) && length(c.ship->position() - point) <= radius) {
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
    auto v = ecs::call<&get_centre>(collision.handle);
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

    if (e.check(collision.handle, point, category)) {
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
    auto v = ecs::call<&get_centre>(collision.handle);
    fixed w = collision.bounding_width;

    // TODO: same as above.
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }

    if (e.check(collision.handle, point, category)) {
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

bool SimInterface::is_on_screen(const vec2& point) const {
  return all(greaterThanEqual(point, vec2{0})) && all(lessThanEqual(point, vec2{kSimDimensions}));
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
  IShip* ship = nullptr;
  IShip* dead = nullptr;
  fixed ship_dist = 0;
  fixed dead_dist = 0;

  for (IShip* s : players()) {
    auto d = length_squared(s->position() - point);
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

vec2 SimInterface::nearest_player_position(const vec2& point) const {
  return nearest_player(point)->shape().centre;
}

vec2 SimInterface::nearest_player_direction(const vec2& point) const {
  return normalise(nearest_player_position(point) - point);
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

ecs::handle SimInterface::create_legacy(std::unique_ptr<Ship> ship) {
  auto p = ship.get();
  auto h = internals_->index.create();
  h.emplace<LegacyShip>().ship = std::move(ship);
  h.emplace<Update>().update = [](ecs::handle h, SimInterface&) {
    static_cast<Ship*>(h.get<LegacyShip>()->ship.get())->update();
  };
  h.emplace<Render>().render = [](ecs::const_handle h, const SimInterface&) {
    static_cast<Ship*>(h.get<LegacyShip>()->ship.get())->render();
  };
  p->set_handle(h);
  return h;
}

void SimInterface::add_particle(const ii::particle& particle) {
  internals_->particles.emplace_back(particle);
}

void SimInterface::explosion(const glm::vec2& v, const glm::vec4& c, std::uint32_t time,
                             const std::optional<glm::vec2>& towards) {
  auto n = towards ? random(2) + 1 : random(8) + 8;
  for (std::uint32_t i = 0; i < n; ++i) {
    auto dir = from_polar(random_fixed().to_float() * 2 * glm::pi<float>(), 6.f);
    if (towards && *towards - v != glm::vec2{0.f}) {
      dir = glm::normalize(*towards - v);
      float angle =
          std::atan2(dir.y, dir.x) + (random_fixed().to_float() - 0.5f) * glm::pi<float>() / 4;
      dir = from_polar(angle, 6.f);
    }
    add_particle({v, c, dir, time + random(8)});
  }
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

void SimInterface::play_sound(sound s, const vec2& position, bool random, float volume) {
  if (random) {
    play_sound(s, volume * (.5f * random_fixed().to_float() + .5f),
               2.f * position.x.to_float() / kSimDimensions.x - 1.f);
  } else {
    play_sound(s, volume, 2.f * position.x.to_float() / kSimDimensions.x - 1.f);
  }
}

void SimInterface::play_sound(sound s, float volume, float pan, float repitch) const {
  auto& aggregation = internals_->sound_output[s];
  ++aggregation.count;
  aggregation.volume += volume;
  aggregation.pan += pan;
  aggregation.pitch = repitch;
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