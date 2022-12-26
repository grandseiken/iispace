#include "game/logic/v0/player/shot.h"
#include "game/geometry/shapes/box.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

struct PlayerShot;
ecs::handle spawn_player_shot(SimInterface& sim, const vec2& position, const PlayerShot& shot_data);

struct shot_mod_data {
  static constexpr std::uint32_t kBaseDamage = 8u;
  static constexpr std::uint32_t kCloseCombatDamage = 6u;
  static constexpr std::uint32_t kCloseCombatShotCount = 12u;
  static constexpr fixed kBaseSpeed = 75_fx / 8_fx;
  static constexpr fixed kCloseCombatMaxDistance = 320_fx;
  static constexpr fixed kCloseCombatSpreadAngle = pi<fixed> / 5;
  static constexpr fixed kSniperSplitDistance = 320_fx;
  static constexpr fixed kSniperSplitRotation = pi<fixed> / 32;

  shot_mod_data(const PlayerLoadout& loadout)
  : sniper_splits_remaining{loadout.has(mod_id::kSniperWeapon) ? 1u : 0u} {
    if (loadout.has(mod_id::kCloseCombatWeapon)) {
      max_distance = kCloseCombatMaxDistance;
      damage = kCloseCombatDamage;
    }
  }

  bool penetrating = false;  // TODO: unused.
  fixed speed = kBaseSpeed;
  fixed distance_travelled = 0u;
  std::optional<fixed> max_distance;
  std::uint32_t sniper_splits_remaining = 0u;
  std::uint32_t damage = kBaseDamage;
};

struct PlayerShot : ecs::component {
  static constexpr auto z = colour::kZPlayerShot;
  static constexpr auto style = geom::sline(colour::kZero, z);
  using shape = standard_transform<
      geom::box<vec2{4_fx, 4_fx}, geom::sline(colour::kOutline, colour::kZOutline)>,
      geom::box_colour_p<vec2{2 + 1_fx / 2_fx, 2 + 1_fx / 2_fx}, 2, style>,
      geom::box_colour_p<vec2{1 + 1_fx / 4_fx, 1 + 1_fx / 4_fx}, 3, style>>;

  std::tuple<vec2, fixed, cvec4, cvec4> shape_parameters(const Transform& transform) const {
    auto c_dark = colour;
    c_dark.a = .2f;
    return {transform.centre, transform.rotation, colour, c_dark};
  }

  ecs::entity_id player;
  std::uint32_t player_number = 0;
  bool is_predicted = false;
  vec2 direction{0};
  cvec4 colour{0.f};
  shot_mod_data data;

  PlayerShot(ecs::entity_id player, std::uint32_t player_number, bool is_predicted,
             const PlayerLoadout& loadout, const vec2& direction)
  : player{player}
  , player_number{player_number}
  , is_predicted{is_predicted}
  , direction{normalise(direction)}
  , colour{v0_player_colour(player_number)}
  , data{loadout} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (data.sniper_splits_remaining &&
        data.distance_travelled >= shot_mod_data::kSniperSplitDistance) {
      colour = colour::kWhite1;
      --data.sniper_splits_remaining;
      auto& s0 = *spawn_player_shot(sim, transform.centre, *this).get<PlayerShot>();
      s0.direction = rotate(s0.direction, shot_mod_data::kSniperSplitRotation);
      auto& s1 = *spawn_player_shot(sim, transform.centre, *this).get<PlayerShot>();
      s1.direction = rotate(s1.direction, -shot_mod_data::kSniperSplitRotation);
    }
    transform.move(direction * data.speed);
    data.distance_travelled += data.speed;
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4, 4} + sim.dimensions()));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    bool shielded = false;
    bool destroy = false;
    std::optional<vec2> destroy_particles;
    if (data.max_distance && data.distance_travelled >= *data.max_distance) {
      destroy = true;
      destroy_particles = direction;
    }
    auto collision = sim.collide_point(transform.centre,
                                       shape_flag::kVulnerable | shape_flag::kWeakVulnerable |
                                           shape_flag::kShield | shape_flag::kWeakShield);
    for (const auto& e : collision) {
      if (e.h.has<Destroy>() ||
          !(e.hit_mask & (shape_flag::kVulnerable | shape_flag::kWeakVulnerable))) {
        continue;
      }
      auto type = is_predicted ? damage_type::kPredicted
          : data.penetrating   ? damage_type::kPenetrating
                               : damage_type::kNormal;
      if (const auto* status = e.h.get<EnemyStatus>(); status && status->shielded_ticks > 4u) {
        shielded = true;
      }
      ecs::call_if<&Health::damage>(e.h, sim, shielded ? 0u : data.damage, type, player,
                                    transform.centre - 2 * direction * data.speed);
      if ((shielded || !(e.hit_mask & shape_flag::kWeakVulnerable)) && !data.penetrating) {
        destroy = true;
        if (!shielded) {
          destroy_particles = -direction;
        }
        break;
      }
    }
    if (!destroy) {
      for (const auto& e : collision) {
        if (+(e.hit_mask & shape_flag::kShield) ||
            (!data.penetrating && +(e.hit_mask & shape_flag::kWeakShield))) {
          shielded = true;
          destroy = true;
        }
      }
    }

    if (destroy_particles) {
      auto e = sim.emit(resolve_key::predicted());
      auto& r = sim.random(random_source::kAesthetic);
      for (std::uint32_t i = 0; i < 2 + r.uint(2); ++i) {
        // TODO: position + reflect direction could be more accurate with cleverer hit info.
        auto v =
            from_polar(angle(to_float(*destroy_particles)) + 2.f * (.5f - r.fixed().to_float()),
                       2.f * r.fixed().to_float() + 2.f);
        e.add(particle{
            .position = to_float(transform.centre - direction * data.speed / 2),
            .velocity = v,
            .colour = colour,
            .data = dot_particle{.radius = 1.f, .line_width = 1.5f},
            .end_time = 8 + r.uint(8),
            .flash_time = 3,
            .fade = true,
        });
      }
    }
    if (destroy) {
      h.emplace<Destroy>();
    }
  }
};
DEBUG_STRUCT_TUPLE(PlayerShot, player, player_number, is_predicted, direction, data.penetrating,
                   data.speed, data.distance_travelled, data.sniper_splits_remaining);

ecs::handle
spawn_player_shot(SimInterface& sim, const vec2& position, const PlayerShot& shot_data) {
  auto h = create_ship_default<PlayerShot>(sim, position);
  h.add(shot_data);
  return h;
}

}  // namespace

void spawn_player_shot(SimInterface& sim, ecs::handle player, const vec2& position,
                       const vec2& direction) {
  const auto& p = *player.get<Player>();
  const auto& loadout = *player.get<PlayerLoadout>();

  if (loadout.has(mod_id::kCloseCombatWeapon)) {
    for (std::uint32_t i = 0; i < shot_mod_data::kCloseCombatShotCount; ++i) {
      auto d = rotate(direction,
                      (sim.random_fixed() - 1_fx / 2) * shot_mod_data::kCloseCombatSpreadAngle);
      spawn_player_shot(sim, position,
                        PlayerShot{player.id(), p.player_number, p.is_predicted, loadout, d});
    }
    return;
  }
  spawn_player_shot(sim, position,
                    PlayerShot{player.id(), p.player_number, p.is_predicted, loadout, direction});
}

}  // namespace ii::v0
