#include "game/logic/v0/player/player.h"
#include "game/logic/geometry/shapes/box.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/player/bubble.h"
#include "game/logic/v0/player/shot.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

struct PlayerLogic : ecs::component {
  static constexpr fixed kPlayerSpeed = 5_fx * 15_fx / 16_fx;
  static constexpr float kZIndex = 96.f;

  static constexpr std::uint32_t kReviveTime = 100;
  static constexpr std::uint32_t kShotTimer = 5;

  using box_shapes =
      geom::translate<8, 0, geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<2, 2, 2>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<1, 1, 3>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<3, 3, 3>>>;
  using shape =
      standard_transform<geom::ngon_colour_p<18, 3, 2>,
                         geom::rotate<fixed_c::pi, geom::ngon_colour_p<9, 3, 2>>, box_shapes>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4>
  shape_parameters(const Player& pc, const Transform& transform) const {
    auto colour = pc.is_killed      ? glm::vec4{0.f}
        : invulnerability_timer % 2 ? glm::vec4{1.f}
                                    : player_colour(pc.player_number);
    auto c_dark = colour;
    c_dark.a = std::min(c_dark.a, .2f);
    return {transform.centre, transform.rotation, colour, c_dark};
  };

  PlayerLogic(const vec2& target) : fire_target{target} {}
  std::optional<ecs::entity_id> bubble_id;
  std::uint32_t invulnerability_timer = kReviveTime;
  std::uint32_t fire_timer = 0;
  vec2 fire_target{0};
  bool fire_target_trail = true;

  void update(ecs::handle h, Player& pc, Transform& transform, Render& render, SimInterface& sim) {
    pc.speed = kPlayerSpeed;
    const auto& input = sim.input(pc.player_number);
    auto old_fire_target = fire_target;
    if (input.target_absolute) {
      fire_target = *input.target_absolute;
    } else if (input.target_relative) {
      fire_target = transform.centre + *input.target_relative;
    }
    fire_target = glm::clamp(fire_target, vec2{0}, sim.dimensions());
    fire_target_trail = length_squared(fire_target - old_fire_target) <= 64 * 64;
    fire_timer && --fire_timer;

    // Temporary death.
    auto dim = sim.dimensions();
    if (pc.is_killed) {
      if (!bubble_id) {
        return;
      }
      if (auto h = sim.index().get(*bubble_id); h) {
        transform = *h->get<Transform>();
        return;
      }
      return;
    }
    invulnerability_timer && --invulnerability_timer;

    // Movement.
    if (length(input.velocity) > fixed_c::hundredth) {
      auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
      auto av = angle(v);
      if (abs(angle_diff(av, transform.rotation)) > fixed_c::pi / 3) {
        render.clear_trails = true;
      }
      transform.set_rotation(angle(v));
      transform.centre = max(vec2{0}, min(dim, kPlayerSpeed * v + transform.centre));
    }

    // Shots.
    auto shot = fire_target - transform.centre;
    if (length(shot) > 0 && !fire_timer && input.keys & input_frame::kFire) {
      spawn_player_shot(sim, transform.centre, h, shot,
                        /* penetrating */ input.keys & input_frame::kBomb);
      fire_timer = kShotTimer;

      auto& random = sim.random(random_source::kAesthetic);
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

  void post_update(ecs::handle h, Player& pc, const Transform& transform, SimInterface& sim) {
    if (bubble_id && !sim.index().get(*bubble_id)) {
      bubble_id.reset();
      pc.is_killed = false;
      invulnerability_timer = kReviveTime;
      sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn))
          .rumble(pc.player_number, 20, 0.f, 1.f)
          .play(sound::kPlayerRespawn, transform.centre);
    }
  }

  void damage(ecs::handle h, Player& pc, Transform& transform, SimInterface& sim) {
    if (pc.is_killed || invulnerability_timer) {
      return;
    }

    // TODO: this can still emit wrong deaths for _local_ players! Since enemies can be in different
    // positions... do we need to make _all_ players predicted for purposes of getting hit?
    auto e = sim.emit(resolve_key::local(pc.player_number));
    explode_entity_shapes<PlayerLogic>(h, e);
    explode_entity_shapes<PlayerLogic>(h, e, glm::vec4{1.f}, 14);
    explode_entity_shapes<PlayerLogic>(h, e, std::nullopt, 20);
    destruct_entity_lines<PlayerLogic>(h, e, transform.centre, 32);

    ++pc.death_count;
    pc.is_killed = true;
    e.rumble(pc.player_number, 30, .5f, .5f).play(sound::kPlayerDestroy, transform.centre);
  }

  void render(const Player& pc, std::vector<render::shape>& output) const {
    if (pc.is_killed) {
      return;
    }
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

  std::optional<render::player_info> render_info(const Player& pc, const SimInterface& sim) const {
    render::player_info info;
    info.colour = player_colour(pc.player_number);
    info.score = 0;
    return info;
  }
};
DEBUG_STRUCT_TUPLE(PlayerLogic, invulnerability_timer, fire_timer, fire_target);

}  // namespace

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  auto h = create_ship_default<PlayerLogic>(sim, position);
  h.add(
      Player{.player_number = player_number, .render_info = ecs::call<&PlayerLogic::render_info>});
  h.add(PlayerLogic{position + vec2{0, -48}});
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
