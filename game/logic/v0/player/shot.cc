#include "game/logic/v0/player/shot.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

struct PlayerShot : ecs::component {
  static constexpr fixed kSpeed = 10_fx * 15_fx / 16_fx;

  static constexpr auto z = colour::kZPlayerShot;
  static constexpr auto style = geom::sline(colour::kZero, z);
  using shape = standard_transform<
      geom::box<vec2{4_fx, 4_fx}, geom::sline(colour::kOutline, colour::kZOutline)>,
      geom::box_colour_p<vec2{2 + 1_fx / 2_fx, 2 + 1_fx / 2_fx}, 2, style>,
      geom::box_colour_p<vec2{1 + 1_fx / 4_fx, 1 + 1_fx / 4_fx}, 3, style>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4> shape_parameters(const Transform& transform) const {
    auto c_dark = colour;
    c_dark.a = .2f;
    return {transform.centre, transform.rotation, colour, c_dark};
  }

  ecs::entity_id player;
  std::uint32_t player_number = 0;
  bool is_predicted = false;
  vec2 velocity{0};
  bool penetrating = false;
  glm::vec4 colour{0.f};

  PlayerShot(ecs::entity_id player, std::uint32_t player_number, bool is_predicted,
             const vec2& direction, bool penetrating)
  : player{player}
  , player_number{player_number}
  , is_predicted{is_predicted}
  , velocity{normalise(direction) * kSpeed}
  , penetrating{penetrating} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    colour = penetrating && sim.random(random_source::kLegacyAesthetic).rbool()
        ? colour::kWhite0
        : v0_player_colour(player_number);
    transform.move(velocity);
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4, 4} + sim.dimensions()));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    bool shielded = false;
    bool destroy = false;
    bool destroy_particles = false;
    auto collision = sim.collision_list(transform.centre,
                                        shape_flag::kVulnerable | shape_flag::kWeakVulnerable |
                                            shape_flag::kShield | shape_flag::kWeakShield);
    for (const auto& e : collision) {
      if (e.h.has<Destroy>() ||
          !(e.hit_mask & (shape_flag::kVulnerable | shape_flag::kWeakVulnerable))) {
        continue;
      }
      auto type = is_predicted ? damage_type::kPredicted
          : penetrating        ? damage_type::kPenetrating
                               : damage_type::kNormal;
      if (const auto* status = e.h.get<EnemyStatus>(); status && status->shielded_ticks > 4u) {
        shielded = true;
      }
      ecs::call_if<&Health::damage>(e.h, sim, shielded ? 0u : 8u, type, player,
                                    transform.centre - 2 * velocity);
      if ((shielded || !(e.hit_mask & shape_flag::kWeakVulnerable)) && !penetrating) {
        destroy = true;
        destroy_particles = !shielded;
        break;
      }
    }
    if (!destroy) {
      for (const auto& e : collision) {
        if (+(e.hit_mask & shape_flag::kShield) ||
            (!penetrating && +(e.hit_mask & shape_flag::kWeakShield))) {
          shielded = true;
          destroy = true;
        }
      }
    }

    if (destroy) {
      if (destroy_particles) {
        auto e = sim.emit(resolve_key::predicted());
        auto& r = sim.random(random_source::kAesthetic);
        for (std::uint32_t i = 0; i < 2 + r.uint(2); ++i) {
          // TODO: position + reflect direction could be more accurate with cleverer hit info.
          auto v = from_polar(angle(to_float(-velocity)) + 2.f * (.5f - r.fixed().to_float()),
                              2.f * r.fixed().to_float() + 2.f);
          e.add(particle{
              .position = to_float(transform.centre - velocity / 2),
              .velocity = v,
              .colour = colour,
              .data = dot_particle{.radius = 1.f, .line_width = 1.5f},
              .end_time = 8 + r.uint(8),
              .flash_time = 3,
              .fade = true,
          });
        }
      }
      h.emplace<Destroy>();
    }
  }
};
DEBUG_STRUCT_TUPLE(PlayerShot, player, player_number, velocity, penetrating);

}  // namespace

void spawn_player_shot(SimInterface& sim, const vec2& position, ecs::handle player,
                       const vec2& direction, bool penetrating) {
  const auto& p = *player.get<Player>();
  auto h = create_ship_default<PlayerShot>(sim, position);
  h.add(PlayerShot{player.id(), p.player_number, p.is_predicted, direction, penetrating});
}

}  // namespace ii::v0
