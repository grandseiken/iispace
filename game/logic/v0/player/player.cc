#include "game/logic/v0/player/player.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/output.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/lib/particles.h"
#include "game/logic/v0/lib/ship_template.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/powerup.h"
#include "game/logic/v0/player/shot.h"

namespace ii::v0 {
namespace {
using namespace geom2;

struct player_mod_data {
  static constexpr std::uint32_t kShieldRefillTimer = 60 * 24;
  static constexpr std::uint32_t kBombDoubleTriggerTimer = 45;

  std::uint32_t shield_refill_timer = 0;
  std::uint32_t bomb_double_trigger_timer = 0;
};
DEBUG_STRUCT_TUPLE(player_mod_data, shield_refill_timer, bomb_double_trigger_timer);

struct PlayerLogic : ecs::component {
  static constexpr std::uint32_t kReviveTime = 150;
  static constexpr std::uint32_t kShieldTime = 50;
  static constexpr std::uint32_t kInputTimer = 30;
  static constexpr fixed kPlayerSpeed = 5_fx * 15_fx / 16_fx;
  static constexpr fixed kBoundingWidth = 0;
  static constexpr auto kFlags = shape_flag::kNone;
  static constexpr auto z = colour::kZPlayer;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    n.add(ngon{.dimensions = nd(21, 3),
               .line = {.colour0 = key{'o'}, .z = colour::kZOutline, .width = 3.f}});
    n.add(ngon{.dimensions = nd(18, 3),
               .line = {.colour0 = key{'c'}, .z = z, .width = 1.5f},
               .fill = {.colour0 = key{'f'}, .z = z}});

    n.add(rotate{pi<fixed>})
        .add(ngon{.dimensions = nd(9, 3), .line = {.colour0 = key{'c'}, .z = z}});
    auto& t = n.add(translate{vec2{8, 0}}).add(rotate{multiply(-1_fx, key{'r'})});
    t.add(box{.dimensions = vec2{2}, .line = {.colour0 = key{'c'}, .z = z}});
    t.add(box{.dimensions = vec2{1}, .line = {.colour0 = key{'d'}, .z = z}});
    t.add(box{.dimensions = vec2{3}, .line = {.colour0 = key{'d'}, .z = z}});
  }

  void
  set_parameters(const Player& pc, const Transform& transform, parameter_set& parameters) const {
    auto colour = colour::kZero;
    if (!pc.is_killed) {
      colour = v0_player_colour(pc.player_number);
      if (invulnerability_timer) {
        cvec4 c{colour.x, 0.f, 1.f, 1.f};
        colour =
            glm::mix(colour, c, .375f + .125f * sin(invulnerability_timer / (2.f * pi<float>)));
      }
    }
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, colour)
        .add(key{'d'}, colour::alpha(colour, .2f))
        .add(key{'f'}, colour::alpha(colour, colour::kFillAlpha0))
        .add(key{'o'}, colour::alpha(colour::kOutline, colour.a));
  }

  PlayerLogic(const vec2& target) : fire_target{target} {}
  std::optional<ecs::entity_id> bubble_id;
  std::uint32_t invulnerability_timer = kReviveTime;
  std::uint32_t fire_timer = 0;
  std::uint32_t bomb_timer = 0;
  std::uint32_t click_timer = 0;
  std::uint32_t render_timer = 0;
  std::uint32_t nametag_timer = invulnerability_timer;
  std::uint32_t fire_target_render_timer = 0;
  player_mod_data data;
  vec2 fire_target{0};
  bool mod_upgrade_chosen = false;

  void update(ecs::handle h, Player& pc, Transform& transform, const PlayerLoadout& loadout,
              SimInterface& sim) {
    pc.is_clicking = false;
    pc.speed = kPlayerSpeed;
    ++render_timer;
    if (!mod_upgrade_chosen && pc.mod_upgrade_chosen) {
      invulnerability_timer = kReviveTime;
    }
    mod_upgrade_chosen = pc.mod_upgrade_chosen;

    const auto& input = sim.input(pc.player_number);
    if (input.target_absolute) {
      fire_target = *input.target_absolute;
      fire_target_render_timer = 1;
    } else if (input.target_relative) {
      if (pc.is_killed) {
        if (input.keys & input_frame::kFire) {
          fire_target += *input.target_relative / 8;
          fire_target_render_timer = 50;
        } else if (fire_target_render_timer) {
          --fire_target_render_timer;
        }
      } else {
        fire_target = transform.centre + *input.target_relative;
      }
    }
    fire_target = glm::clamp(fire_target, vec2{0}, sim.dimensions());
    fire_timer && --fire_timer;
    bomb_timer && --bomb_timer;
    click_timer && --click_timer;

    // Temporary death.
    auto dim = sim.dimensions();
    if (pc.is_killed) {
      if (!bubble_id) {
        return;
      }
      if (auto h = sim.index().get(*bubble_id); h) {
        transform = *h->get<Transform>();
      }
      return;
    }

    nametag_timer && --nametag_timer;
    invulnerability_timer && --invulnerability_timer;
    if (loadout.has(mod_id::kShieldRefill)) {
      if (pc.shield_count >= 1u) {
        pc.shield_count = 1u;
        data.shield_refill_timer = 0u;
      }
      if (!pc.shield_count && ++data.shield_refill_timer >= player_mod_data::kShieldRefillTimer) {
        pc.shield_count = 1u;
        data.shield_refill_timer = 0u;
        auto e = sim.emit(resolve_key::predicted());
        e.play(sound::kPowerupOther, transform.centre);
        e.rumble(pc.player_number, 10, .25f, .75f);
        e.explosion(to_float(transform.centre), colour::kWhite0);
      }
    }

    // Movement.
    if (length(input.velocity) > fixed_c::hundredth) {
      auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
      transform.set_rotation(angle(v));
      transform.centre = max(vec2{0}, min(dim, kPlayerSpeed * v + transform.centre));
    }

    // Click.
    if (!pc.is_predicted && !click_timer && input.keys & input_frame::kClick) {
      pc.is_clicking = true;
      click_timer = kInputTimer;
    }

    // Bomb.
    if (data.bomb_double_trigger_timer && !--data.bomb_double_trigger_timer) {
      trigger_bomb(h, pc, loadout, transform.centre, sim);
      bomb_timer = kInputTimer;
    }
    if (!pc.is_predicted && pc.bomb_count && !bomb_timer && !data.bomb_double_trigger_timer &&
        input.keys & input_frame::kBomb) {
      trigger_bomb(h, pc, loadout, transform.centre, sim);
      --pc.bomb_count;
      bomb_timer = kInputTimer;
      if (loadout.has(mod_id::kBombDoubleTrigger)) {
        data.bomb_double_trigger_timer = player_mod_data::kBombDoubleTriggerTimer;
      }
    }

    // Shots.
    auto shot_direction = fire_target - transform.centre;
    if (length_squared(shot_direction) > 0 && !fire_timer && input.keys & input_frame::kFire) {
      spawn_shot(h, loadout, transform.centre, shot_direction, sim);
      fire_timer = loadout.shot_fire_timer();
    }

    // Damage.
    auto collision = sim.collide(check_point(shape_flag::kDangerous, transform.centre));
    if (!pc.is_predicted && !collision.empty()) {
      std::optional<vec2> source;
      for (const auto& c : collision) {
        if (auto* t = c.h.get<Transform>(); t) {
          source = t->centre;
          break;
        }
      }
      damage(h, pc, loadout, transform, source.value_or(transform.centre), sim);
    }
  }

  void post_update(ecs::handle h, Player& pc, const PlayerLoadout& loadout,
                   const Transform& transform, Render& render, SimInterface& sim) {
    if (bubble_id && !sim.index().get(*bubble_id)) {
      // TODO: small bomb on respawn.
      bubble_id.reset();
      pc.is_killed = false;
      render.clear_trails = true;
      data = {};
      invulnerability_timer = nametag_timer = kReviveTime;
      if (loadout.has(mod_id::kShieldRespawn)) {
        pc.shield_count = loadout.max_shield_capacity(sim);
      }
      sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn))
          .rumble(pc.player_number, 20, 0.f, 1.f)
          .play(sound::kPlayerRespawn, transform.centre);
    }
  }

  void damage(ecs::handle h, Player& pc, const PlayerLoadout& loadout, Transform& transform,
              const vec2& source, SimInterface& sim) {
    if (pc.is_killed || invulnerability_timer) {
      return;
    }

    // TODO: this can still emit wrong deaths for _local_ players! Since enemies can be in different
    // positions... do we need to make _all_ players predicted for purposes of getting hit?
    auto e = sim.emit(resolve_key::local(pc.player_number));
    if (pc.shield_count) {
      e.rumble(pc.player_number, 15, 0.f, 1.f).play(sound::kPlayerShield, transform.centre);
      --pc.shield_count;
      data.shield_refill_timer = 0;
      invulnerability_timer = kShieldTime;
      if (loadout.has(mod_id::kCloseCombatShield)) {
        bomb_timer = kInputTimer;
        trigger_bomb(h, pc, loadout, transform.centre, sim);
        if (loadout.has(mod_id::kBombDoubleTrigger)) {
          data.bomb_double_trigger_timer = player_mod_data::kBombDoubleTriggerTimer;
        }
      }
      return;
    }

    auto& r = resolve_entity_shape<default_shape_definition<PlayerLogic>>(h, sim);
    explode_shapes(e, r);
    explode_shapes(e, r, colour::kWhite0, 14);
    explode_shapes(e, r, std::nullopt, 20);
    destruct_lines(e, r, to_float(source), 32);
    explode_volumes(e, r, to_float(source));

    ++pc.death_count;
    pc.bomb_count = 0;
    pc.is_killed = true;
    data = {};
    e.rumble(pc.player_number, 30, .5f, .5f).play(sound::kPlayerDestroy, transform.centre);
  }

  void spawn_shot(ecs::handle h, const PlayerLoadout& loadout, const vec2& position,
                  const vec2& direction, SimInterface& sim) const {
    if (loadout.has(mod_id::kBackShots)) {
      spawn_player_shot(sim, h, position, -direction);
    }
    if (loadout.has(mod_id::kFrontShots)) {
      static constexpr fixed kFrontShotSpreadDistance = 6;
      auto p = kFrontShotSpreadDistance * normalise(perpendicular(direction)) / 2;
      spawn_player_shot(sim, h, position + p, direction);
      spawn_player_shot(sim, h, position - p, direction);
    } else {
      spawn_player_shot(sim, h, position, direction);
    }

    auto& random = sim.random(random_source::kAesthetic);
    float volume = .5f * random.fixed().to_float() + .5f;
    float pitch = (random.fixed().to_float() - 1.f) / 12.f;
    float pan = 2.f * position.x.to_float() / sim.dimensions().x.to_int() - 1.f;
    sim.emit(resolve_key::predicted()).play(sound::kPlayerFire, volume, pan, pitch);
  }

  void trigger_bomb(ecs::const_handle h, Player& pc, const PlayerLoadout& loadout,
                    const vec2& position, SimInterface& sim) const {
    static constexpr std::uint32_t kBombDamage = 400;
    static constexpr fixed kBombRadius = 180;
    auto radius = kBombRadius * loadout.bomb_radius_multiplier();

    auto c = v0_player_colour(pc.player_number);
    auto e = sim.emit(resolve_key::local(pc.player_number));
    auto& random = sim.random(random_source::kAesthetic);
    auto& r = resolve_entity_shape<default_shape_definition<PlayerLogic>>(h, sim);

    // TODO: fx bomb explosion. Work out how to do really big explosions.
    explode_shapes(e, r, colour::kWhite0, 18);
    explode_shapes(e, r, c, 21);
    explode_shapes(e, r, colour::kWhite0, 24);
    for (std::uint32_t i = 0; i < 64; ++i) {
      auto v = position + from_polar(2 * i * pi<fixed> / 64, radius);
      auto& r = resolve_shape<&construct_shape>(sim, [&](parameter_set& parameters) {
        set_parameters(pc, {{}, v, 0_fx}, parameters);
      });
      explode_shapes(e, r, (i % 2) ? c : colour::kWhite0, 8 + random.uint(8) + random.uint(8),
                     to_float(position));
    }

    e.rumble(pc.player_number, 20, 1.f, .5f).play(sound::kExplosion, position);
    for (const auto& c : sim.collide(check_ball(
             shape_flag::kVulnerable | shape_flag::kWeakVulnerable | shape_flag::kBombVulnerable,
             position, radius))) {
      if (auto* health = c.h.get<Health>(); health) {
        health->damage(c.h, sim, kBombDamage, damage_type::kBomb, h.id(), position);
      }
    }
  }

  static void construct_fire_target(node& root) {
    root.add(translate{key{'v'}})
        .add(ngon{.dimensions = nd(8, 4),
                  .style = ngon_style::kPolystar,
                  .line = {.colour0 = key{'c'}, .z = colour::kZPlayerCursor},
                  .tag = render::tag_t{'t'}});
  }

  static void construct_shield_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    for (std::uint32_t i = 0; i < 3; ++i) {
      auto& r = n.add(rotate{i * 2_fx * pi<fixed> / 3});
      r.add(ngon{
          .dimensions = {.radius = key{'d'}, .sides = 18, .segments = 4},
          .line = sline(colour::kWhite1, colour::kZPlayerPowerup),
          .tag = render::tag_t{'s'},
      });
      r.add(ngon{
          .dimensions = {.radius = key{'D'}, .sides = 18, .segments = 4},
          .line = sline(colour::kOutline, colour::kZOutline, 2.f),
          .tag = render::tag_t{'s'},
      });
    }
  }

  static void construct_bomb_shape(node& root) {
    root.add(translate_rotate{key{'v'}, key{'r'}})
        .add(translate{key{'V'}})
        .add(ngon{
            .dimensions = nd(3, 6),
            .line = sline(colour::kWhite0, colour::kZPlayerPowerup),
            .tag = render::tag_t{'b'},
        });
  }

  void render(const Player& pc, const Transform& transform, std::vector<render::shape>& output,
              const SimInterface& sim) const {
    if (!pc.is_killed || fire_target_render_timer) {
      auto& r = resolve_shape<&construct_fire_target>(sim, [&](parameter_set& parameters) {
        parameters.add(key{'v'}, fire_target).add(key{'c'}, v0_player_colour(pc.player_number));
      });
      render_shape(output, r);
    }
    if (pc.is_killed) {
      return;
    }

    for (std::uint32_t i = 0; i < pc.shield_count; ++i) {
      auto rotation = pi<fixed> * (render_timer % 240) / 120_fx;
      auto& r = resolve_shape<&construct_shield_shape>(sim, [&](parameter_set& parameters) {
        parameters.add(key{'v'}, transform.centre)
            .add(key{'r'}, rotation)
            .add(key{'d'}, 20 + 3_fx * i / 2_fx)
            .add(key{'D'}, 20 + 3_fx * (i + 1) / 2_fx);
      });
      render_shape(output, r);
    }

    for (std::uint32_t i = 0; i < pc.bomb_count; ++i) {
      auto t = 6_fx - (std::min(4u, (std::max(pc.bomb_count, 2u) - 2u)) / 2_fx);
      vec2 v{-14, (pc.bomb_count - 1) * (t / 2) - t * i};

      auto& r = resolve_shape<&construct_bomb_shape>(sim, [&](parameter_set& parameters) {
        parameters.add(key{'v'}, transform.centre)
            .add(key{'r'}, transform.rotation)
            .add(key{'V'}, v);
      });
      render_shape(output, r);
    }
  }

  void render_panel(const Player& pc, const Transform& transform,
                    std::vector<render::combo_panel>& output, const SimInterface& sim) const {
    if (nametag_timer) {
      render_player_name_panel(pc.player_number, transform, output, sim);
    }
  }

  std::optional<player_info> render_info(const Player& pc, const SimInterface& sim) const {
    player_info info;
    info.colour = v0_player_colour(pc.player_number);
    info.score = 0;
    return info;
  }
};
DEBUG_STRUCT_TUPLE(PlayerLogic, bubble_id, invulnerability_timer, fire_timer, bomb_timer,
                   click_timer, render_timer, nametag_timer, fire_target_render_timer, data,
                   fire_target, mod_upgrade_chosen);

}  // namespace

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  auto h = create_ship_default<PlayerLogic>(sim, position);
  h.add(
      Player{.player_number = player_number, .render_info = ecs::call<&PlayerLogic::render_info>});
  h.add(PlayerLogic{position + vec2{0, -48}});
  h.add(PlayerLoadout{});
  h.add(PostUpdate{.post_update = ecs::call<&PlayerLogic::post_update>});
}

void respawn_players(SimInterface& sim) {
  sim.index().iterate_dispatch<Player>([&](ecs::handle h, const Player& pc, PlayerLogic& logic) {
    if (pc.is_killed && !logic.bubble_id) {
      logic.bubble_id = spawn_player_bubble(sim, h).id();
    }
  });
}

}  // namespace ii::v0
