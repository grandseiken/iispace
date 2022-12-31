#include "game/logic/sim/sim_interface.h"
#include "game/logic/ecs/call.h"
#include "game/logic/legacy/components.h"
#include "game/logic/sim/components.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/sim_internals.h"

namespace ii {
namespace {

RandomEngine& engine(SimInternals& internals, random_source s) {
  if (internals.conditions.compatibility == compatibility_level::kLegacy) {
    return s == random_source::kAesthetic ? internals.aesthetic_random
                                          : internals.game_state_random;
  }
  return s == random_source::kGameState   ? internals.game_state_random
      : s == random_source::kGameSequence ? internals.game_sequence_random
                                          : internals.aesthetic_random;
}
}  // namespace

RandomEngine& EmitHandle::random() {
  return sim->random(random_source::kAesthetic);
}

EmitHandle& EmitHandle::set_delay_ticks(std::uint32_t ticks) {
  e->delay_ticks = ticks;
  return *this;
}

EmitHandle& EmitHandle::background_fx(background_fx_change change) {
  e->background_fx = change;
  return *this;
}

EmitHandle& EmitHandle::add(particle particle) {
  e->particles.emplace_back(particle);
  return *this;
}

EmitHandle& EmitHandle::explosion(const fvec2& v, const cvec4& c, std::uint32_t time,
                                  const std::optional<fvec2>& towards, std::optional<float> speed) {
  auto& r = sim->random(random_source::kLegacyAesthetic);
  auto& ra = sim->random(random_source::kAesthetic);
  auto n = towards ? r.rbool() + 1 : r.uint(8) + 8;
  for (std::uint32_t i = 0; i < n; ++i) {
    float rspeed = speed ? *speed * (1.f + ra.fixed().to_float()) : 6.f;
    auto dir = from_polar(r.fixed().to_float() * 2 * pi<float>, rspeed);
    if (towards && *towards - v != fvec2{0.f}) {
      dir = glm::normalize(*towards - v);
      float angle = std::atan2(dir.y, dir.x) + (r.fixed().to_float() - 0.5f) * pi<float> / 4;
      dir = from_polar(angle, rspeed);
    }
    dot_particle dot;
    if (speed && ra.rbool()) {
      dot.line_width = 2.f;
      dot.radius = 1.f;
    }
    dot.rotation = ra.fixed().to_float() * pi<float>;
    dot.angular_velocity = ra.fixed().to_float() - .5f;
    add(particle{
        .position = v,
        .velocity = dir,
        .colour = c,
        .data = dot,
        .end_time = time + r.uint(8),
    });
  }
  return *this;
}

EmitHandle& EmitHandle::rumble(std::uint32_t player, std::uint32_t time_ticks, float lf, float hf) {
  e->rumble.emplace_back(rumble_out{player, time_ticks, lf, hf});
  return *this;
}

EmitHandle& EmitHandle::rumble_all(std::uint32_t time_ticks, float lf, float hf) {
  for (std::uint32_t i = 0; i < sim->player_count(); ++i) {
    rumble(i, time_ticks, lf, hf);
  }
  return *this;
}

EmitHandle& EmitHandle::play(sound s, float volume, float pan, float repitch) {
  auto& se = e->sounds.emplace_back();
  se.sound_id = s;
  se.volume = volume;
  se.pan = pan;
  se.pitch = repitch;
  return *this;
}

EmitHandle& EmitHandle::play(sound s, const vec2& position, float volume, float repitch) {
  return play(
      s, volume,
      2.f * (position.x.to_float() + 1.f / 16) / (sim->dimensions().x.to_int() - 1.f / 8) - 1.f,
      repitch);
}

EmitHandle& EmitHandle::play_random(sound s, const vec2& position, float volume) {
  auto& r = sim->random(random_source::kLegacyAesthetic);
  return play(s, position, volume * (.5f * r.fixed().to_float() + .5f));
}

const initial_conditions& SimInterface::conditions() const {
  return internals_->conditions;
}

vec2 SimInterface::dimensions() const {
  return internals_->dimensions;
}

bool SimInterface::is_legacy() const {
  return internals_->conditions.compatibility == compatibility_level::kLegacy;
}

input_frame& SimInterface::input(std::uint32_t player_number) {
  if (player_number < internals_->input_frames.size()) {
    return internals_->input_frames[player_number];
  }
  static input_frame kDefaultFrame;
  return kDefaultFrame;
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

RandomEngine& SimInterface::random(random_source s) {
  return engine(*internals_, s);
}

std::uint32_t SimInterface::random_state(random_source s) const {
  return engine(*internals_, s).state();
}

std::uint32_t SimInterface::random(std::uint32_t max) {
  return random(random_source::kGameState).uint(max);
}

fixed SimInterface::random_fixed() {
  return random(random_source::kGameState).fixed();
}

bool SimInterface::random_bool() {
  return random(random_source::kGameState).rbool();
}

bool SimInterface::collide_point_any(const vec2& point, shape_flag mask) const {
  return internals_->collision_index->collide_point_any(point, mask);
}

bool SimInterface::collide_line_any(const vec2& a, const vec2& b, shape_flag mask) const {
  return internals_->collision_index->collide_line_any(a, b, mask);
}

bool SimInterface::collide_ball_any(const vec2& centre, fixed radius, shape_flag mask) const {
  return internals_->collision_index->collide_ball_any(centre, radius, mask);
}

bool SimInterface::collide_convex_any(std::span<const vec2> convex, shape_flag mask) const {
  return internals_->collision_index->collide_convex_any(convex, mask);
}

auto SimInterface::collide_point(const vec2& point, shape_flag mask) const
    -> std::vector<collision_info> {
  return internals_->collision_index->collide_point(point, mask);
}

auto SimInterface::collide_line(const vec2& a, const vec2& b, shape_flag mask) const
    -> std::vector<collision_info> {
  return internals_->collision_index->collide_line(a, b, mask);
}

auto SimInterface::collide_ball(const vec2& centre, fixed radius, shape_flag mask) const
    -> std::vector<collision_info> {
  return internals_->collision_index->collide_ball(centre, radius, mask);
}

auto SimInterface::collide_convex(std::span<const vec2> convex, shape_flag mask) const
    -> std::vector<collision_info> {
  return internals_->collision_index->collide_convex(convex, mask);
}

bool SimInterface::is_on_screen(const vec2& point) const {
  return all(greaterThanEqual(point, vec2{0})) && all(lessThanEqual(point, dimensions()));
}

vec2 SimInterface::rotate_compatibility(const vec2& v, fixed theta) const {
  return conditions().compatibility == compatibility_level::kLegacy ? rotate_legacy(v, theta)
                                                                    : rotate(v, theta);
}

void SimInterface::in_range(const vec2& point, fixed distance, ecs::component_id cid,
                            std::size_t max_n, std::vector<range_info>& output) {
  internals_->collision_index->in_range(point, distance, cid, max_n, output);
}

std::uint32_t SimInterface::get_lives() const {
  if (auto* data = internals_->global_entity_handle->get<legacy::GlobalData>(); data) {
    return data->lives;
  }
  return 0;
}

std::uint32_t SimInterface::player_count() const {
  return conditions().player_count;
}

std::uint32_t SimInterface::alive_players() const {
  return player_count() - killed_players();
}

std::uint32_t SimInterface::killed_players() const {
  std::uint32_t result = 0;
  internals_->index.iterate_dispatch<Player>([&](const Player& pc) {
    if (pc.is_killed) {
      ++result;
    }
  });
  return result;
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
        if ((d < alive_distance || !nearest_alive) && !p.is_killed) {
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
    if ((d < alive_distance || !nearest_alive) && !p.is_killed) {
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

EmitHandle SimInterface::emit(const resolve_key& key) {
  auto& e = internals_->output.entries.emplace_back();
  e.key = key;
  return {*this, e.e};
}

void SimInterface::trigger(const run_event& event) {
  internals_->results.events.emplace_back(event);
}

}  // namespace ii