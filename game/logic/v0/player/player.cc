#include "game/logic/v0/player/player.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/ngon.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/output.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/powerup.h"
#include "game/logic/v0/player/shot.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

struct player_mod_data {
  std::uint32_t shield_refill_timer = 0;
};

struct PlayerLogic : ecs::component {
  static constexpr fixed kPlayerSpeed = 5_fx * 15_fx / 16_fx;

  static constexpr std::uint32_t kReviveTime = 100;
  static constexpr std::uint32_t kShieldTime = 50;
  static constexpr std::uint32_t kInputTimer = 30;
  static constexpr std::uint32_t kShieldRefillTimer = 60 * 25;

  static constexpr auto z = colour::kZPlayer;
  static constexpr auto style = geom::sline(colour::kZero, z);
  using box_shapes = geom::translate<
      8, 0, geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<vec2{2, 2}, 2, style>>,
      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<vec2{1, 1}, 3, style>>,
      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<vec2{3, 3}, 3, style>>>;
  using shape = standard_transform<
      geom::ngon_colour_p<geom::nd(21, 3), 5,
                          geom::nline(colour::kOutline, colour::kZOutline, 3.f)>,
      geom::ngon_colour_p2<geom::nd(18, 3), 2, 4, geom::nline(colour::kZero, z, 1.5f)>,
      geom::rotate<pi<fixed>, geom::ngon_colour_p<geom::nd(9, 3), 2>>, box_shapes>;

  std::tuple<vec2, fixed, cvec4, cvec4, cvec4, cvec4>
  shape_parameters(const Player& pc, const Transform& transform) const {
    auto colour = colour::kZero;
    if (!pc.is_killed) {
      colour = v0_player_colour(pc.player_number);
      if (invulnerability_timer) {
        cvec4 c{colour.x, 0.f, 1.f, 1.f};
        colour =
            glm::mix(colour, c, .375f + .125f * sin(invulnerability_timer / (2.f * pi<float>)));
      }
    }
    auto c_dark = colour;
    c_dark.a = std::min(c_dark.a, .2f);
    return {transform.centre,
            transform.rotation,
            colour,
            c_dark,
            colour::alpha(colour, std::min(colour.a, colour::kFillAlpha0)),
            colour::alpha(colour::kOutline, colour.a)};
  };

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
    if (loadout.has(mod_id::kShieldRefill) && ++data.shield_refill_timer >= kShieldRefillTimer) {
      pc.shield_count = 1u;
      data.shield_refill_timer = 0u;
      auto e = sim.emit(resolve_key::predicted());
      e.play(sound::kPowerupOther, transform.centre);
      e.rumble(pc.player_number, 10, .25f, .75f);
      e.explosion(to_float(transform.centre), colour::kWhite0);
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
    if (!pc.is_predicted && pc.bomb_count && !bomb_timer && input.keys & input_frame::kBomb) {
      trigger_bomb(h, pc, loadout, transform.centre, sim);
      --pc.bomb_count;
      bomb_timer = kInputTimer;
    }

    // Shots.
    auto shot_direction = fire_target - transform.centre;
    if (length_squared(shot_direction) > 0 && !fire_timer && input.keys & input_frame::kFire) {
      spawn_shot(h, loadout, transform.centre, shot_direction, sim);
      fire_timer = loadout.shot_fire_timer();
    }

    // Damage.
    auto collision = sim.collide_point(transform.centre, shape_flag::kDangerous);
    if (!pc.is_predicted && !collision.empty()) {
      std::optional<vec2> source;
      for (const auto& c : collision) {
        if (auto* t = c.h.get<Transform>(); t) {
          source = t->centre;
          break;
        }
      }
      damage(h, pc, transform, source.value_or(transform.centre), sim);
    }
  }

  void post_update(ecs::handle h, Player& pc, const PlayerLoadout& loadout,
                   const Transform& transform, Render& render, SimInterface& sim) {
    if (bubble_id && !sim.index().get(*bubble_id)) {
      bubble_id.reset();
      pc.is_killed = false;
      render.clear_trails = true;
      data.shield_refill_timer = 0;
      invulnerability_timer = nametag_timer = kReviveTime;
      if (loadout.has(mod_id::kShieldRespawn)) {
        pc.shield_count = loadout.max_shield_capacity(sim);
      }
      sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn))
          .rumble(pc.player_number, 20, 0.f, 1.f)
          .play(sound::kPlayerRespawn, transform.centre);
    }
  }

  void
  damage(ecs::handle h, Player& pc, Transform& transform, const vec2& source, SimInterface& sim) {
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
      return;
    }

    explode_entity_shapes<PlayerLogic>(h, e);
    explode_entity_shapes<PlayerLogic>(h, e, colour::kWhite0, 14);
    explode_entity_shapes<PlayerLogic>(h, e, std::nullopt, 20);
    destruct_entity_lines<PlayerLogic>(h, e, source, 32);

    ++pc.death_count;
    pc.bomb_count = 0;
    pc.is_killed = true;
    data.shield_refill_timer = 0;
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

    auto c = v0_player_colour(pc.player_number);
    auto e = sim.emit(resolve_key::local(pc.player_number));
    auto& random = sim.random(random_source::kAesthetic);
    explode_entity_shapes<PlayerLogic>(h, e, colour::kWhite0, 18);
    explode_entity_shapes<PlayerLogic>(h, e, c, 21);
    explode_entity_shapes<PlayerLogic>(h, e, colour::kWhite0, 24);

    auto radius = kBombRadius * loadout.bomb_radius_multiplier();
    for (std::uint32_t i = 0; i < 64; ++i) {
      auto v = position + from_polar(2 * i * pi<fixed> / 64, radius);
      auto parameters = shape_parameters(pc, {{}, v, 0_fx});
      explode_shapes<shape>(e, parameters, (i % 2) ? c : colour::kWhite0,
                            8 + random.uint(8) + random.uint(8), position);
    }

    e.rumble(pc.player_number, 20, 1.f, .5f).play(sound::kExplosion, position);
    for (const auto& c : sim.collide_ball(position, radius,
                                          shape_flag::kVulnerable | shape_flag::kWeakVulnerable)) {
      if (auto* health = c.h.get<Health>(); health) {
        health->damage(c.h, sim, kBombDamage, damage_type::kBomb, h.id(), position);
      }
    }
  }

  void
  render(const Player& pc, const Transform& transform, std::vector<render::shape>& output) const {
    if (!pc.is_killed || fire_target_render_timer) {
      output.emplace_back(render::shape{
          .origin = to_float(fire_target),
          .colour = v0_player_colour(pc.player_number),
          .z_index = 100.f,
          .s_index = 't',
          .data = render::ngon{.radius = 8, .sides = 4, .style = render::ngon_style::kPolystar},
      });
    }
    if (pc.is_killed) {
      return;
    }

    for (std::uint32_t i = 0; i < pc.shield_count; ++i) {
      auto rotation =
          static_cast<float>(pi<float> * static_cast<float>(render_timer % 240) / 120.f);
      for (std::uint32_t j = 0; j < 3; ++j) {
        output.emplace_back(render::shape{
            .origin = to_float(transform.centre),
            .rotation = rotation + static_cast<float>(j) * 2.f * pi<float> / 3.f,
            .colour = colour::kWhite1,
            .z_index = colour::kZPlayerPowerup,
            .s_index = 's',
            .data = render::ngon{.radius = 20.f + 1.5f * i, .sides = 18, .segments = 4},
        });
        output.emplace_back(render::shape{
            .origin = to_float(transform.centre),
            .rotation = rotation + static_cast<float>(j) * 2.f * pi<float> / 3.f,
            .colour = colour::kOutline,
            .z_index = colour::kZOutline,
            .s_index = 's',
            .data =
                render::ngon{
                    .radius = 21.5f + 1.5f * i, .sides = 18, .segments = 4, .line_width = 1.5f},
        });
      }
    }

    for (std::uint32_t i = 0; i < pc.bomb_count; ++i) {
      auto t = 6_fx - (std::min(4u, (std::max(pc.bomb_count, 2u) - 2u)) / 2_fx);
      vec2 v{-14, (pc.bomb_count - 1) * (t / 2) - t * i};
      output.emplace_back(render::shape{
          .origin = to_float(transform.centre + rotate(v, transform.rotation)),
          .rotation = transform.rotation.to_float(),
          .colour = colour::kWhite0,
          .z_index = colour::kZPlayerPowerup,
          .s_index = 'b',
          .data = render::ngon{.radius = 3.f, .sides = 6},
      });
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
                   click_timer, render_timer, data.shield_refill_timer, nametag_timer,
                   fire_target_render_timer, fire_target, mod_upgrade_chosen);

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
