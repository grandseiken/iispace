#include "game/logic/v0/player/shot.h"
#include "game/common/enum.h"
#include "game/geometry/shapes/box.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/lib/particles.h"
#include "game/logic/v0/lib/ship_template.h"
#include "game/logic/v0/player/loadout.h"

namespace ii::v0 {
namespace {
enum class shot_flags : std::uint32_t {
  kNone = 0,
  kPenetrating = 0b0001,  // TODO: unused.
  kInflictStun = 0b0010,
  kHomingShots = 0b0100,
  kSniperSplit = 0b1000,
};
}  // namespace
}  // namespace ii::v0

namespace ii {
template <>
struct bitmask_enum<v0::shot_flags> : std::true_type {};
}  // namespace ii

namespace ii::v0 {
namespace {

struct PlayerShot;
ecs::handle spawn_player_shot(SimInterface& sim, const vec2& position, const PlayerShot& shot_data);

struct shot_mod_data {
  static constexpr std::uint32_t kBaseDamage = 8u;
  static constexpr std::uint32_t kLightningDamage = 12u;  // TODO: maybe too much. 10?
  static constexpr std::uint32_t kCloseCombatDamage = 6u;
  static constexpr std::uint32_t kSniperSplitDamage = 6u;
  static constexpr std::uint32_t kCloseCombatShotCount = 14u;
  static constexpr fixed kBaseSpeed = 75_fx / 8_fx;
  static constexpr fixed kCloseCombatMaxDistance = 320_fx;
  static constexpr fixed kCloseCombatSpreadAngle = pi<fixed> / 5;
  static constexpr fixed kHomingScanRadius = 120_fx;
  static constexpr fixed kHomingAngleChange = pi<fixed> / 60;
  static constexpr fixed kSniperSplitDistance = 320_fx;
  static constexpr fixed kSniperSplitRotation = pi<fixed> / 32;

  shot_mod_data(const PlayerLoadout& loadout) {
    if (loadout.has(mod_id::kHomingShots)) {
      flags |= shot_flags::kHomingShots;
    }
    if (loadout.has(mod_id::kCloseCombatWeapon)) {
      max_distance = kCloseCombatMaxDistance;
      damage = kCloseCombatDamage;
    }
    if (loadout.has(mod_id::kSniperWeapon)) {
      flags |= shot_flags::kSniperSplit;
    }
    if (loadout.has(mod_id::kLightningWeapon)) {
      // TODO: needs some kind of visual effect + stun status effect.
      damage = kLightningDamage;
      flags |= shot_flags::kInflictStun;
    }
  }

  shot_flags flags = shot_flags::kNone;
  fixed speed = kBaseSpeed;
  fixed distance_travelled = 0u;
  std::optional<fixed> max_distance;
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
    if (+(data.flags & shot_flags::kSniperSplit) &&
        data.distance_travelled >= shot_mod_data::kSniperSplitDistance) {
      colour = colour::kWhite1;
      data.damage = shot_mod_data::kSniperSplitDamage;
      data.flags &= ~shot_flags::kSniperSplit;
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

    if (+(data.flags & shot_flags::kHomingShots) && !(data.flags & shot_flags::kSniperSplit)) {
      std::optional<vec2> target;
      fixed min_d_sq = 0;
      auto c_list = sim.collide_ball(transform.centre, shot_mod_data::kHomingScanRadius,
                                     shape_flag::kVulnerable | shape_flag::kWeakVulnerable);
      for (const auto& c : c_list) {
        for (const auto& vc : c.shape_centres) {
          auto d_sq = length_squared(vc - transform.centre);
          auto a_diff = angle_diff(angle(direction), angle(vc - transform.centre));
          if (abs(a_diff) < pi<fixed> / 2 && (!target || d_sq < min_d_sq)) {
            min_d_sq = d_sq;
            target = vc;
          }
        }
      }

      if (target) {
        // TODO: upgrade to super-threatseeker mod that applies RC-smooth (e.g. 15/16) towards
        // target direction? Or just use that for cluster super.
        auto a_diff = angle_diff(angle(direction), angle(*target - transform.centre));
        auto a = abs(a_diff);
        if (a < shot_mod_data::kHomingAngleChange) {
          direction = normalise(*target - transform.centre);
        } else {
          direction =
              rotate(direction,
                     (a_diff > 0 ? 1_fx : -1_fx) * std::min(a, shot_mod_data::kHomingAngleChange));
        }
      }
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
      auto type = is_predicted                       ? damage_type::kPredicted
          : +(data.flags & shot_flags::kPenetrating) ? damage_type::kPenetrating
                                                     : damage_type::kNormal;
      if (const auto* status = e.h.get<EnemyStatus>(); status && status->shielded_ticks > 4u) {
        shielded = true;
      }
      ecs::call_if<&Health::damage>(e.h, sim, shielded ? 0u : data.damage, type, player,
                                    transform.centre - 2 * direction * data.speed);
      if (!shielded && +(data.flags & shot_flags::kInflictStun)) {
        if (auto* status = e.h.get<EnemyStatus>(); status) {
          status->inflict_stun();
        }
      }
      if ((shielded || !(e.hit_mask & shape_flag::kWeakVulnerable)) &&
          !(data.flags & shot_flags::kPenetrating)) {
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
            (!(data.flags & shot_flags::kPenetrating) && +(e.hit_mask & shape_flag::kWeakShield))) {
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
DEBUG_STRUCT_TUPLE(PlayerShot, player, player_number, is_predicted, direction, data.flags,
                   data.speed, data.distance_travelled, data.max_distance, data.damage);

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
