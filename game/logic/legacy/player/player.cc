#include "game/logic/legacy/player/player.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/geometry/node_conditional.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/legacy/player/ai_player.h"
#include "game/logic/ship/ship_template.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/render.h"
#include <algorithm>
#include <utility>

namespace ii::legacy {
namespace {
constexpr std::uint32_t kMagicShotCount = 120;

struct Shot : ecs::component {
  static constexpr fixed kSpeed = 10;
  static constexpr float kZIndex = 64.f;

  using shape = standard_transform<geom::box_colour_p<2, 2, 2>, geom::box_colour_p<1, 1, 3>,
                                   geom::box_colour_p<3, 3, 3>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4> shape_parameters(const Transform& transform) const {
    auto c_dark = colour;
    c_dark.a = .2f;
    return {transform.centre, transform.rotation, colour, c_dark};
  }

  ecs::entity_id player;
  std::uint32_t player_number = 0;
  bool is_predicted = false;
  vec2 velocity{0};
  bool magic = false;
  glm::vec4 colour{0.f};

  Shot(ecs::entity_id player, std::uint32_t player_number, bool is_predicted, const vec2& direction,
       bool magic)
  : player{player}
  , player_number{player_number}
  , is_predicted{is_predicted}
  , velocity{normalise(direction) * kSpeed}
  , magic{magic} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    colour = magic && sim.random(random_source::kLegacyAesthetic).rbool()
        ? glm::vec4{1.f}
        : player_colour(player_number);
    transform.move(velocity);
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4, 4} + sim.dimensions()));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    bool destroy = false;
    bool destroy_particles = false;
    auto generation = sim.index().generation();
    auto collision = sim.collision_list(
        transform.centre, shape_flag::kVulnerable | shape_flag::kShield | shape_flag::kWeakShield);
    for (const auto& e : collision) {
      if (+(e.hit_mask & shape_flag::kVulnerable)) {
        auto type = is_predicted ? damage_type::kPredicted
            : magic              ? damage_type::kMagic
                                 : damage_type::kNone;
        ecs::call_if<&Health::damage>(e.h, sim, 1, type, player, transform.centre - 2 * velocity);
        if (!magic) {
          destroy = true;
          destroy_particles = true;
        }
      }
    }

    if (!destroy) {
      // Compatibility: need to rerun the collision check if new entities might have spawned.
      if (generation != sim.index().generation()) {
        collision =
            sim.collision_list(transform.centre, shape_flag::kShield | shape_flag::kWeakShield);
      }
      for (const auto& e : collision) {
        if (!e.h.has<Destroy>() &&
            (+(e.hit_mask & shape_flag::kShield) ||
             (!magic && +(e.hit_mask & shape_flag::kWeakShield)))) {
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
DEBUG_STRUCT_TUPLE(Shot, player, player_number, velocity, magic);

void spawn_shot(SimInterface& sim, const vec2& position, ecs::handle player, const vec2& direction,
                bool magic) {
  const auto& p = *player.get<Player>();
  create_ship<Shot>(sim, position)
      .add(Shot{player.id(), p.player_number, p.is_predicted, direction, magic});
}

struct PlayerLogic : ecs::component {
  static constexpr fixed kBombRadius = 180;
  static constexpr fixed kBombBossRadius = 280;
  static constexpr float kZIndex = 96.f;

  static constexpr std::uint32_t kReviveTime = 100;
  static constexpr std::uint32_t kShieldTime = 50;
  static constexpr std::uint32_t kShotTimer = 4;

  using shield = geom::if_p<2, geom::ngon_colour_p<16, 10, 6>>;
  using bomb = geom::if_p<
      3, geom::translate<-8, 0, geom::rotate<fixed_c::pi, geom::polystar_colour_p<6, 5, 6>>>>;
  using box_shapes =
      geom::translate<8, 0, geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<2, 2, 4>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<1, 1, 5>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<3, 3, 5>>>;
  using shape = standard_transform<geom::ngon_colour_p<16, 3, 4>,
                                   geom::rotate<fixed_c::pi, geom::ngon_colour_p<8, 3, 4>>,
                                   box_shapes, shield, bomb>;

  std::tuple<vec2, fixed, bool, bool, glm::vec4, glm::vec4, glm::vec4>
  shape_parameters(const Player& pc, const Transform& transform) const {
    auto colour = !should_render(pc) ? glm::vec4{0.f}
        : invulnerability_timer % 2  ? glm::vec4{1.f}
                                     : player_colour(pc.player_number);
    auto c_dark = colour;
    c_dark.a = std::min(c_dark.a, .2f);
    auto powerup_colour = !should_render(pc) ? glm::vec4{0.f} : glm::vec4{1.f};
    return {transform.centre, transform.rotation, pc.has_shield, pc.has_bomb, colour,
            c_dark,           powerup_colour};
  };

  PlayerLogic(const SimInterface& sim, const vec2& target)
  : is_what_mode{sim.conditions().mode == game_mode::kWhat}, fire_target{target} {}
  bool is_what_mode = false;
  std::uint32_t invulnerability_timer = kReviveTime;
  std::uint32_t fire_timer = 0;
  vec2 fire_target{0};
  bool fire_target_trail = true;

  void update(ecs::handle h, Player& pc, Transform& transform, Render& render, SimInterface& sim) {
    const auto& input = sim.input(pc.player_number);
    auto old_fire_target = fire_target;
    if (input.target_absolute) {
      fire_target = *input.target_absolute;
    } else if (input.target_relative) {
      fire_target = transform.centre + *input.target_relative;
    }
    if (!sim.is_legacy()) {
      fire_target = glm::clamp(fire_target, vec2{0}, sim.dimensions());
    }
    fire_target_trail = length_squared(fire_target - old_fire_target) <= 64 * 64;
    fire_timer = (fire_timer + 1) % kShotTimer;

    // Temporary death.
    if (pc.kill_timer > 1 && --pc.kill_timer) {
      return;
    }

    auto dim = sim.dimensions();
    auto& global_data = *sim.global_entity().get<GlobalData>();
    auto& kill_queue = global_data.player_kill_queue;
    if (pc.kill_timer) {
      if (global_data.lives && !kill_queue.empty() && kill_queue.front() == pc.player_number) {
        --global_data.lives;
        pc.kill_timer = 0;
        kill_queue.erase(kill_queue.begin());
        invulnerability_timer = kReviveTime;
        render.trails.clear();
        transform.centre = {(1 + pc.player_number) * dim.x.to_int() / (1 + sim.player_count()),
                            dim.y / 2};
        sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn))
            .rumble(pc.player_number, 20, 0.f, 1.f)
            .play(sound::kPlayerRespawn, transform.centre);
      }
      return;
    }
    invulnerability_timer && --invulnerability_timer;

    // Movement.
    if (length(input.velocity) > fixed_c::hundredth) {
      auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
      auto av = angle(v);
      if (abs(angle_diff(av, transform.rotation)) > fixed_c::pi / 3) {
        render.trails.clear();
      }
      transform.set_rotation(angle(v));
      transform.centre = max(vec2{0}, min(dim, kPlayerSpeed * v + transform.centre));
    }

    // Bombs.
    if (!pc.is_predicted && pc.has_bomb && input.keys & input_frame::kBomb) {
      auto c = player_colour(pc.player_number);
      pc.has_bomb = false;

      auto e = sim.emit(resolve_key::local(pc.player_number));
      explosion(h, std::nullopt, e, glm::vec4{1.f}, 16);
      explosion(h, std::nullopt, e, c, 32);
      explosion(h, std::nullopt, e, glm::vec4{1.f}, 48);

      for (std::uint32_t i = 0; i < 64; ++i) {
        auto v = transform.centre + from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
        auto& random = sim.random(random_source::kLegacyAesthetic);
        explosion(h, v, e, (i % 2) ? c : glm::vec4{1.f}, 8 + random.uint(8) + random.uint(8),
                  transform.centre);
      }

      e.rumble(pc.player_number, 20, 1.f, .5f).play(sound::kExplosion, transform.centre);

      sim.index().iterate_dispatch_if<Enemy>(
          [&](ecs::handle eh, const Enemy& e, const Transform& e_transform, Health& health,
              Boss* boss) {
            if (length(e_transform.centre - transform.centre) > kBombBossRadius) {
              return;
            }
            if (boss || length(e_transform.centre - transform.centre) <= kBombRadius) {
              health.damage(eh, sim, Player::kBombDamage, damage_type::kBomb, h.id(),
                            transform.centre);
            }
            if (!boss && e.score_reward) {
              pc.add_score(sim, 0);
            }
          },
          /* include_new */ false);
    }

    // Shots.
    auto shot = fire_target - transform.centre;
    if (length(shot) > 0 && !fire_timer && input.keys & input_frame::kFire) {
      spawn_shot(sim, transform.centre, h, shot, pc.magic_shot_count != 0);
      pc.magic_shot_count && --pc.magic_shot_count;

      auto& random = sim.random(random_source::kLegacyAesthetic);
      float volume = .5f * random.fixed().to_float() + .5f;
      float pitch = (random.fixed().to_float() - 1.f) / 12.f;
      float pan = 2.f * transform.centre.x.to_float() / dim.x.to_int() - 1.f;
      sim.emit(resolve_key::predicted()).play(sound::kPlayerFire, volume, pan, pitch);
    }

    // Damage.
    if (!pc.is_predicted && sim.any_collision(transform.centre, shape_flag::kDangerous)) {
      damage(h, pc, transform, sim);
    }
  }

  void damage(ecs::handle h, Player& pc, Transform& transform, SimInterface& sim) {
    if (pc.kill_timer || invulnerability_timer) {
      return;
    }

    // TODO: this can still emit wrong deaths for _local_ players! Since enemies can be in different
    // positions... do we need to make _all_ players predicted for purposes of getting hit?
    auto e = sim.emit(resolve_key::local(pc.player_number));
    if (pc.has_shield) {
      e.rumble(pc.player_number, 15, 0.f, 1.f).play(sound::kPlayerShield, transform.centre);
      pc.has_shield = false;
      invulnerability_timer = kShieldTime;
      return;
    }

    explosion(h, std::nullopt, e);
    explosion(h, std::nullopt, e, glm::vec4{1.f}, 14);
    explosion(h, std::nullopt, e, std::nullopt, 20);
    destruct_entity_lines<PlayerLogic>(h, e, transform.centre, 32);

    pc.magic_shot_count = 0;
    pc.multiplier = 1;
    pc.multiplier_count = 0;
    pc.kill_timer = kReviveTime;
    ++pc.death_count;
    if (pc.has_shield || pc.has_bomb) {
      pc.has_shield = false;
      pc.has_bomb = false;
    }
    sim.global_entity().get<GlobalData>()->player_kill_queue.push_back(pc.player_number);
    e.rumble(pc.player_number, 30, .5f, .5f).play(sound::kPlayerDestroy, transform.centre);
  }

  void explosion(ecs::handle h, const std::optional<vec2>& position_override, EmitHandle& e,
                 const std::optional<glm::vec4>& colour = std::nullopt, std::uint32_t time = 8,
                 const std::optional<vec2>& towards = std::nullopt) const {
    auto parameters = get_shape_parameters<PlayerLogic>(h);
    if (position_override) {
      std::get<0>(parameters) = *position_override;
    }
    explode_shapes<shape>(e, parameters, colour, time, towards);
  }

  void render(const Player& pc, std::vector<render::shape>& output) const {
    if (should_render(pc)) {
      auto c = player_colour(pc.player_number);
      auto t = to_float(fire_target);
      output.emplace_back(render::shape{
          .origin = t,
          .colour = c,
          .z_index = 100.f,
          .disable_trail = !fire_target_trail,
          .data = render::ngon{.radius = 8, .sides = 4, .style = render::ngon_style::kPolystar},
      });
    }
  }

  std::optional<render::player_info> render_info(const Player& pc, const SimInterface& sim) const {
    if (sim.conditions().mode == game_mode::kBoss) {
      return std::nullopt;
    }
    render::player_info info;
    info.colour = player_colour(pc.player_number);
    info.score = pc.score;
    info.multiplier = pc.multiplier;
    info.timer = static_cast<float>(pc.magic_shot_count) / kMagicShotCount;
    return info;
  }

  bool should_render(const Player& pc) const {
    return !pc.kill_timer && (!is_what_mode || invulnerability_timer);
  }
};
DEBUG_STRUCT_TUPLE(PlayerLogic, is_what_mode, invulnerability_timer, fire_timer, fire_target);

}  // namespace

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number,
                  bool is_ai) {
  auto h = create_ship<PlayerLogic>(sim, position);
  h.add(
      Player{.player_number = player_number, .render_info = ecs::call<&PlayerLogic::render_info>});
  h.add(PlayerLogic{sim, position + vec2{0, -48}});
  if (is_ai) {
    h.add(AiPlayer{});
  }
}

std::optional<input_frame> ai_think(const SimInterface& sim, ecs::handle h) {
  if (auto* ai = h.get<AiPlayer>(); ai) {
    return ecs::call<&AiPlayer::think>(h, sim);
  }
  return std::nullopt;
}

}  // namespace ii::legacy
