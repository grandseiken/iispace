#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"

namespace ii {
namespace {
constexpr std::uint32_t kGbBaseHp = 700;
constexpr std::uint32_t kGbTimer = 250;
constexpr std::uint32_t kGbAttackTime = 100;

constexpr glm::vec4 c0 = colour_hue360(280, .7f);
constexpr glm::vec4 c1 = colour_hue360(280, .5f, .6f);
constexpr glm::vec4 c2 = colour_hue360(270, .2f);

struct GhostWall : ecs::component {
  static constexpr fixed kBoundingWidth = kSimDimensions.x;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr fixed kSpeed = 3 + 1_fx / 2;
  static constexpr fixed kOffsetH = 20 + kSimDimensions.y / 2;

  template <std::uint32_t length>
  using gw_box =
      geom::compound<geom::box<length, 10, c0, shape_flag::kDangerous | shape_flag::kShield>,
                     geom::box<length, 7, c0>, geom::box<length, 4, c0>>;
  using gw_horizontal_base = geom::rotate<fixed_c::pi / 2, gw_box<kSimDimensions.y / 2>>;
  template <fixed Y0, fixed Y1>
  using gw_horizontal_align = geom::compound<geom::translate<0, Y0, gw_horizontal_base>,
                                             geom::translate<0, Y1, gw_horizontal_base>>;
  using shape = geom::translate_p<
      0,
      geom::conditional_p<
          1, geom::conditional_p<2, gw_box<kSimDimensions.x>, gw_box<kSimDimensions.y / 2>>,
          geom::conditional_p<2, gw_horizontal_align<-100 - kOffsetH, -100 + kOffsetH>,
                              gw_horizontal_align<100 - kOffsetH, 100 + kOffsetH>>>>;

  std::tuple<vec2, bool, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, vertical, gap_swap};
  }

  GhostWall(const vec2& dir, bool vertical, bool gap_swap)
  : dir{dir}, vertical{vertical}, gap_swap{gap_swap} {}

  vec2 dir{0};
  bool vertical = false;
  bool gap_swap = false;

  void update(ecs::handle h, Transform& transform, SimInterface&) {
    if ((dir.x > 0 && transform.centre.x < 32) || (dir.y > 0 && transform.centre.y < 16) ||
        (dir.x < 0 && transform.centre.x >= kSimDimensions.x - 32) ||
        (dir.y < 0 && transform.centre.y >= kSimDimensions.y - 16)) {
      transform.move(dir * kSpeed / 2);
    } else {
      transform.move(dir * kSpeed);
    }

    if ((dir.x > 0 && transform.centre.x > kSimDimensions.x + 10) ||
        (dir.y > 0 && transform.centre.y > kSimDimensions.y + 10) ||
        (dir.x < 0 && transform.centre.x < -10) || (dir.y < 0 && transform.centre.y < -10)) {
      h.emplace<Destroy>();
    }
  }
};

void spawn_ghost_wall_vertical(SimInterface& sim, bool swap, bool no_gap) {
  vec2 position{kSimDimensions.x / 2, swap ? -10 : 10 + kSimDimensions.y};
  vec2 dir{0, swap ? 1 : -1};
  auto h = create_ship<GhostWall>(sim, position);
  add_enemy_health<GhostWall>(h, 0);
  h.add(GhostWall{dir, true, no_gap});
  h.add(Enemy{.threat_value = 1});
}

void spawn_ghost_wall_horizontal(SimInterface& sim, bool swap, bool swap_gap) {
  vec2 position{swap ? -10 : 10 + kSimDimensions.x, kSimDimensions.y / 2};
  vec2 dir{swap ? 1 : -1, 0};
  auto h = create_ship<GhostWall>(sim, position);
  add_enemy_health<GhostWall>(h, 0);
  h.add(GhostWall{dir, false, swap_gap});
  h.add(Enemy{.threat_value = 1});
}

struct GhostMine : ecs::component {
  static constexpr fixed kBoundingWidth = 24;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;

  using shape = standard_transform<
      geom::conditional_p<2, geom::ngon_colour_p<24, 8, 3>,
                          geom::ngon_colour_p<24, 8, 3, geom::ngon_style::kPolygon,
                                              shape_flag::kDangerous | shape_flag::kShield |
                                                  shape_flag::kWeakShield>>,
      geom::ngon_colour_p<20, 8, 3>>;

  std::tuple<vec2, fixed, bool, glm::vec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, timer > 0, (timer / 4) % 2 ? glm::vec4{0.f} : c1};
  }

  std::uint32_t timer = 80;
  ecs::entity_id ghost_boss{0};
  GhostMine(ecs::entity_id ghost_boss) : ghost_boss{ghost_boss} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (timer == 80) {
      explode_entity_shapes<GhostMine>(h, sim);
      transform.set_rotation(sim.random_fixed() * 2 * fixed_c::pi);
    }
    timer && --timer;
    if (sim.any_collision(transform.centre, shape_flag::kEnemyInteraction)) {
      if (sim.random(6) == 0 ||
          (sim.index().contains(ghost_boss) &&
           sim.index().get(ghost_boss)->get<Health>()->is_hp_low() && sim.random(5) == 0)) {
        spawn_big_follow(sim, transform.centre, /* score */ false);
      } else {
        spawn_follow(sim, transform.centre, /* score */ false);
      }
      ecs::call<&Health::damage>(h, sim, 1, damage_type::kNone, std::nullopt);
    }
  }
};

void spawn_ghost_mine(SimInterface& sim, const vec2& position, ::Boss* ghost) {
  auto h = create_ship<GhostMine>(sim, position);
  add_enemy_health<GhostMine>(h, 0);
  h.add(GhostMine{ghost->handle().id()});
  h.add(Enemy{.threat_value = 1});
}

class GhostBoss : public ::Boss {
public:
  GhostBoss(SimInterface& sim);

  void update() override;
  void render() const override;

private:
  bool visible_ = false;
  std::uint32_t vtime_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t attack_time_ = 0;
  std::uint32_t attack_ = 0;
  bool _rdir = false;
  std::uint32_t start_time_ = 120;
  std::uint32_t danger_circle_ = 0;
  std::uint32_t danger_offset1_ = 0;
  std::uint32_t danger_offset2_ = 0;
  std::uint32_t danger_offset3_ = 0;
  std::uint32_t danger_offset4_ = 0;
  bool shot_type_ = false;
};

GhostBoss::GhostBoss(SimInterface& sim)
: Boss{sim, {kSimDimensions.x / 2, kSimDimensions.y / 2}}, attack_time_{kGbAttackTime} {
  add_new_shape<Polygon>(vec2{0}, 32, 8, c1, 0);
  add_new_shape<Polygon>(vec2{0}, 48, 8, c0, 0);

  for (std::uint32_t i = 0; i < 8; ++i) {
    vec2 c = from_polar(i * fixed_c::pi / 4, 48_fx);

    auto* s = add_new_shape<CompoundShape>(c, 0);
    for (std::uint32_t j = 0; j < 8; ++j) {
      vec2 d = from_polar(j * fixed_c::pi / 4, 1_fx);
      s->add_new_shape<Line>(vec2{0}, d * 10, d * 10 * 2, c1, 0);
    }
  }

  add_new_shape<Polygon>(vec2{0}, 24, 8, c0, 0, shape_flag::kNone, Polygon::T::kPolygram);

  for (std::uint32_t n = 0; n < 5; ++n) {
    auto* s = add_new_shape<CompoundShape>(vec2{0}, 0);
    for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
      vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
      s->add_new_shape<Polygon>(d, 16, 8, n ? c2 : c1, 0, shape_flag::kNone, Polygon::T::kPolystar);
      s->add_new_shape<Polygon>(d, 12, 8, n ? c2 : c1);
    }
  }

  for (std::uint32_t n = 0; n < 5; ++n) {
    add_new_shape<CompoundShape>(vec2{0}, 0, shape_flag::kDangerous);
  }
}

void GhostBoss::update() {
  for (std::uint32_t n = 1; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*)shapes()[11 + n].get();
    CompoundShape* c = (CompoundShape*)shapes()[16 + n].get();
    c->clear_shapes();

    for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
      Shape& s1 = *s->shapes()[2 * i];
      Shape& s2 = *s->shapes()[1 + 2 * i];
      auto t = n == 1 ? (i + danger_offset1_) % 11
          : n == 2    ? (i + danger_offset2_) % 7
          : n == 3    ? (i + danger_offset3_) % 17
                      : (i + danger_offset4_) % 10;

      bool b = n == 1 ? (t % 11 == 0 || t % 11 == 5)
          : n == 2    ? (t % 7 == 0)
          : n == 3    ? (t % 17 == 0 || t % 17 == 8)
                      : (t % 10 == 0);

      if (((n == 4 && attack_ != 2) || (n + (n == 3)) & danger_circle_) && visible_ && b) {
        s1.colour = c0;
        s2.colour = c0;
        if (vtime_ == 0) {
          vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
          c->add_new_shape<Polygon>(d, 9, 8, glm::vec4{0.f});
          sim().global_entity().get<GlobalData>()->extra_enemy_warnings.push_back(
              c->convert_point(shape().centre, shape().rotation(), d));
        }
      } else {
        s1.colour = c2;
        s2.colour = c2;
      }
    }
  }
  if (!(attack_ == 2 && attack_time_ < kGbAttackTime * 4 && attack_time_)) {
    shape().rotate(-fixed_c::hundredth * 4);
  }

  for (std::size_t i = 0; i < 8; ++i) {
    shapes()[i + 2]->rotate(fixed_c::hundredth * 4);
  }
  shapes()[10]->rotate(fixed_c::hundredth * 6);
  for (std::uint32_t n = 0; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*)shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 5 - (3_fx / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }

    s = (CompoundShape*)shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 4 - (3_fx / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }
  }

  if (start_time_) {
    handle().get<ii::Boss>()->show_hp_bar = false;
    start_time_--;
    return;
  }

  bool is_hp_low = handle().get<Health>()->is_hp_low();
  if (!attack_time_) {
    ++timer_;
    if (timer_ == 9 * kGbTimer / 10 ||
        (timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
         ((timer_ % 16 == 0 && attack_ == 2) || (timer_ % 32 == 0 && shot_type_)))) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        auto d = from_polar(i * fixed_c::pi / 4 + shape().rotation(), 5_fx);
        spawn_boss_shot(sim(), shape().centre, d, c0);
      }
      play_sound_random(sound::kBossFire);
    }
    if (timer_ != 9 * kGbTimer / 10 && timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
        timer_ % 16 == 0 && (!shot_type_ || attack_ == 2)) {
      auto d = sim().nearest_player_direction(shape().centre);
      if (d != vec2{}) {
        spawn_boss_shot(sim(), shape().centre, d * 5, c0);
        spawn_boss_shot(sim(), shape().centre, d * -5, c0);
        play_sound_random(sound::kBossFire);
      }
    }
  } else {
    attack_time_--;

    if (attack_ == 2) {
      if (attack_time_ >= kGbAttackTime * 4 && attack_time_ % 8 == 0) {
        vec2 pos(sim().random(kSimDimensions.x + 1), sim().random(kSimDimensions.y + 1));
        spawn_ghost_mine(sim(), pos, this);
        play_sound_random(sound::kEnemySpawn);
        danger_circle_ = 0;
      } else if (attack_time_ == kGbAttackTime * 4 - 1) {
        visible_ = true;
        vtime_ = 60;
        add_new_shape<Box>(vec2{kSimDimensions.x / 2 + 32, 0}, kSimDimensions.x / 2, 10, c0, 0);
        add_new_shape<Box>(vec2{kSimDimensions.x / 2 + 32, 0}, kSimDimensions.x / 2, 7, c0, 0);
        add_new_shape<Box>(vec2{kSimDimensions.x / 2 + 32, 0}, kSimDimensions.x / 2, 4, c0, 0);

        add_new_shape<Box>(vec2{-kSimDimensions.x / 2 - 32, 0}, kSimDimensions.x / 2, 10, c0, 0);
        add_new_shape<Box>(vec2{-kSimDimensions.x / 2 - 32, 0}, kSimDimensions.x / 2, 7, c0, 0);
        add_new_shape<Box>(vec2{-kSimDimensions.x / 2 - 32, 0}, kSimDimensions.x / 2, 4, c0, 0);
        play_sound(sound::kBossFire);
      } else if (attack_time_ < kGbAttackTime * 3) {
        if (attack_time_ == kGbAttackTime * 3 - 1) {
          play_sound(sound::kBossAttack);
        }
        shape().rotate((_rdir ? 1 : -1) * 2 * fixed_c::pi / (kGbAttackTime * 6));
        if (!attack_time_) {
          for (std::uint32_t i = 0; i < 6; ++i) {
            destroy_shape(21);
          }
        }
        danger_offset1_ = sim().random(11);
        danger_offset2_ = sim().random(7);
        danger_offset3_ = sim().random(17);
        danger_offset3_ = sim().random(10);
      }
    }

    if (attack_ == 0 && attack_time_ == kGbAttackTime * 2) {
      bool r = sim().random(2) != 0;
      spawn_ghost_wall_horizontal(sim(), r, true);
      spawn_ghost_wall_horizontal(sim(), !r, false);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 2) {
      _rdir = sim().random(2) != 0;
      spawn_ghost_wall_vertical(sim(), !_rdir, false);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 1) {
      spawn_ghost_wall_vertical(sim(), _rdir, true);
    }

    if ((attack_ == 0 || attack_ == 1) && is_hp_low && attack_time_ == kGbAttackTime * 1) {
      auto r = sim().random(4);
      vec2 v = r == 0 ? vec2{-kSimDimensions.x / 4, kSimDimensions.y / 2}
          : r == 1    ? vec2{kSimDimensions.x + kSimDimensions.x / 4, kSimDimensions.y / 2}
          : r == 2    ? vec2{kSimDimensions.x / 2, -kSimDimensions.y / 4}
                      : vec2{kSimDimensions.x / 2, kSimDimensions.y + kSimDimensions.y / 4};
      spawn_big_follow(sim(), v, false);
    }
    if (!attack_time_ && !visible_) {
      visible_ = true;
      vtime_ = 60;
    }
    if (attack_time_ == 0 && attack_ != 2) {
      auto r = sim().random(3);
      danger_circle_ |= 1 + (r == 2 ? 3 : r);
      if (sim().random(2) || is_hp_low) {
        r = sim().random(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
      if (sim().random(2) || is_hp_low) {
        r = sim().random(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
    } else {
      danger_circle_ = 0;
    }
  }

  if (timer_ >= kGbTimer) {
    timer_ = 0;
    visible_ = false;
    vtime_ = 60;
    attack_ = sim().random(3);
    shot_type_ = sim().random(2) == 0;
    shapes()[0]->category = shape_flag::kNone;
    shapes()[1]->category = shape_flag::kNone;
    shapes()[11]->category = shape_flag::kNone;
    if (shapes().size() >= 22) {
      shapes()[21]->category = shape_flag::kNone;
      shapes()[24]->category = shape_flag::kNone;
    }
    play_sound(sound::kBossAttack);

    if (attack_ == 0) {
      attack_time_ = kGbAttackTime * 2 + 50;
    }
    if (attack_ == 1) {
      attack_time_ = kGbAttackTime * 3;
      for (std::uint32_t i = 0; i < sim().player_count(); ++i) {
        spawn_powerup(sim(), shape().centre, powerup_type::kShield);
      }
    }
    if (attack_ == 2) {
      attack_time_ = kGbAttackTime * 6;
      _rdir = sim().random(2) != 0;
    }
  }

  if (vtime_) {
    vtime_--;
    if (!vtime_ && visible_) {
      shapes()[0]->category = shape_flag::kShield;
      shapes()[1]->category =
          shape_flag::kDangerous | shape_flag::kEnemyInteraction | shape_flag::kVulnerable;
      shapes()[11]->category = shape_flag::kDangerous | shape_flag::kEnemyInteraction;
      if (shapes().size() >= 22) {
        shapes()[21]->category = shape_flag::kDangerous | shape_flag::kEnemyInteraction;
        shapes()[24]->category = shape_flag::kDangerous | shape_flag::kEnemyInteraction;
      }
    }
  }
}

void GhostBoss::render() const {
  if ((start_time_ / 4) % 2 == 1) {
    render_with_colour({0.f, 0.f, 0.f, 1.f / 255});
    return;
  }
  if ((visible_ && ((vtime_ / 4) % 2 == 0)) || (!visible_ && ((vtime_ / 4) % 2 == 1))) {
    Boss::render();
    return;
  }

  render_with_colour(c2);
}

}  // namespace

void spawn_ghost_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<GhostBoss>(sim));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2B, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(kGbBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 12,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss2B});
}
}  // namespace ii