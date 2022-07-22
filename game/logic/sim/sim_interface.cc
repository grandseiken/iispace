#include "game/logic/sim/sim_interface.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_internals.h"
#include <glm/gtc/constants.hpp>

namespace ii {
namespace {
GlobalData& global_data(SimInternals& internals) {
  return *internals.global_entity_handle->get<GlobalData>();
}
}  // namespace

const initial_conditions& SimInterface::conditions() const {
  return internals_->conditions;
}

input_frame SimInterface::input(std::uint32_t player_number) {
  if (player_number < internals_->input_frames.size()) {
    return internals_->input_frames[player_number];
  }
  return {};
}

std::uint64_t SimInterface::tick_count() const {
  return internals_->tick_count;
}

const ecs::EntityIndex& SimInterface::index() const {
  return internals_->index;
}

ecs::EntityIndex& SimInterface::index() {
  return internals_->index;
}

ecs::const_handle SimInterface::global_entity() const {
  return *internals_->global_entity_handle;
}

ecs::handle SimInterface::global_entity() {
  return *internals_->global_entity_handle;
}

std::uint32_t SimInterface::random_state() const {
  return internals_->random_engine.state();
}

std::uint32_t SimInterface::random(std::uint32_t max) {
  return internals_->random_engine() % max;
}

fixed SimInterface::random_fixed() {
  return fixed{static_cast<std::int32_t>(internals_->random_engine())} / RandomEngine::rand_max;
}

bool SimInterface::any_collision(const vec2& point, shape_flag mask) const {
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    const auto& e = *collision.collision;
    auto v = collision.transform->centre;
    fixed w = e.bounding_width;

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
    if (!(e.flags & mask)) {
      continue;
    }
    if (+e.check(collision.handle, point, mask)) {
      return true;
    }
  }
  return false;
}

auto SimInterface::collision_list(const vec2& point, shape_flag mask)
    -> std::vector<collision_info> {
  std::vector<collision_info> r;
  fixed x = point.x;
  fixed y = point.y;

  for (const auto& collision : internals_->collisions) {
    const auto& e = *collision.collision;
    auto v = collision.transform->centre;
    fixed w = e.bounding_width;

    // TODO: same as above.
    if (v.x - w > x) {
      break;
    }
    if (v.x + w < x || v.y + w < y || v.y - w > y) {
      continue;
    }
    if (!(e.flags & mask)) {
      continue;
    }
    if (auto hit = e.check(collision.handle, point, mask); + hit) {
      r.emplace_back(collision_info{.h = collision.handle, .hit_mask = hit});
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

std::uint32_t SimInterface::get_lives() const {
  return global_data(*internals_).lives;
}

std::uint32_t SimInterface::player_count() const {
  return conditions().player_count;
}

std::uint32_t SimInterface::alive_players() const {
  return player_count() - killed_players();
}

std::uint32_t SimInterface::killed_players() const {
  return global_data(*internals_).player_kill_queue.size();
}

vec2 SimInterface::nearest_player_position(const vec2& point) const {
  return nearest_player(point).get<Transform>()->centre;
}

vec2 SimInterface::nearest_player_direction(const vec2& point) const {
  return normalise(nearest_player_position(point) - point);
}

ecs::const_handle SimInterface::nearest_player(const vec2& point) const {
  std::optional<ecs::const_handle> nearest_alive;
  std::optional<ecs::const_handle> nearest_dead;
  fixed alive_distance = 0;
  fixed dead_distance = 0;

  index().iterate_dispatch<Player>(
      [&](ecs::const_handle h, const Player& p, const Transform& transform) {
        auto d = length_squared(transform.centre - point);
        if ((d < alive_distance || !nearest_alive) && !p.is_killed()) {
          alive_distance = d;
          nearest_alive = h;
        }
        if (d < dead_distance || !nearest_dead) {
          dead_distance = d;
          nearest_dead = h;
        }
      });
  return nearest_alive ? *nearest_alive : *nearest_dead;
}

ecs::handle SimInterface::nearest_player(const vec2& point) {
  std::optional<ecs::handle> nearest_alive;
  std::optional<ecs::handle> nearest_dead;
  fixed alive_distance = 0;
  fixed dead_distance = 0;

  index().iterate_dispatch<Player>([&](ecs::handle h, const Player& p, const Transform& transform) {
    auto d = length_squared(transform.centre - point);
    if ((d < alive_distance || !nearest_alive) && !p.is_killed()) {
      alive_distance = d;
      nearest_alive = h;
    }
    if (d < dead_distance || !nearest_dead) {
      dead_distance = d;
      nearest_dead = h;
    }
  });
  return nearest_alive ? *nearest_alive : *nearest_dead;
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
                                      std::uint64_t score, std::uint32_t multiplier,
                                      float timer) const {
  internals_->player_output.resize(
      std::max<std::size_t>(internals_->player_output.size(), player_number + 1));
  auto& info = internals_->player_output[player_number];
  info.colour = colour;
  info.score = score;
  info.multiplier = multiplier;
  info.timer = timer;
}

}  // namespace ii