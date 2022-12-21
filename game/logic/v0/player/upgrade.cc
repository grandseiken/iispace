#include "game/logic/v0/player/upgrade.h"
#include "game/common/easing.h"
#include "game/core/icons/mod_icons.h"
#include "game/geometry/iteration.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/loadout_mods.h"
#include "game/logic/v0/ship_template.h"

namespace ii::v0 {
namespace {

enum class upgrade_position : std::uint32_t {
  kL0 = 0,
  kR0 = 1,
  kL1 = 2,
  kR1 = 3,
};

std::vector<render::shape> render_mod_icon(mod_id id, std::uint32_t animation) {
  auto render = [&]<typename S>(S) {
    std::vector<render::shape> shapes;
    auto t = static_cast<fixed>(animation) / 24;
    geom::iterate(S{}, geom::iterate_shapes, std::tuple{t}, geom::transform{},
                  [&](const render::shape& shape) { shapes.emplace_back(shape); });
    return shapes;
  };

  switch (id) {
  case mod_id::kNone:
    break;
  case mod_id::kBackShots:
    return render(icons::generic_weapon<colour::kCategoryGeneral>{});
  case mod_id::kFrontShots:
    return render(icons::generic_weapon<colour::kCategoryGeneral>{});
  case mod_id::kBounceShots:
    return render(icons::generic_weapon<colour::kCategoryGeneral>{});
  case mod_id::kHomingShots:
    return render(icons::generic_weapon<colour::kCategoryGeneral>{});
  case mod_id::kSuperCapacity:
    return render(icons::generic_super<colour::kCategoryGeneral>{});
  case mod_id::kSuperRefill:
    return render(icons::generic_super<colour::kCategoryGeneral>{});
  case mod_id::kBombCapacity:
    return render(icons::generic_bomb<colour::kCategoryGeneral>{});
  case mod_id::kBombRadius:
    return render(icons::generic_bomb<colour::kCategoryGeneral>{});
  case mod_id::kBombSpeedClearCharge:
    return render(icons::generic_bomb<colour::kCategoryGeneral>{});
  case mod_id::kBombDoubleTrigger:
    return render(icons::generic_bomb<colour::kCategoryGeneral>{});
  case mod_id::kShieldCapacity:
    return render(icons::generic_shield<colour::kCategoryGeneral>{});
  case mod_id::kShieldRefill:
    return render(icons::generic_shield<colour::kCategoryGeneral>{});
  case mod_id::kShieldRespawn:
    return render(icons::generic_shield<colour::kCategoryGeneral>{});
  case mod_id::kPowerupDrops:
    return render(icons::generic_bonus<colour::kCategoryGeneral>{});
  case mod_id::kCurrencyDrops:
    return render(icons::generic_bonus<colour::kCategoryGeneral>{});
  case mod_id::kCorruptionWeapon:
    return render(icons::generic_weapon<colour::kCategoryCorruption>{});
  case mod_id::kCorruptionSuper:
    return render(icons::generic_super<colour::kCategoryCorruption>{});
  case mod_id::kCorruptionBomb:
    return render(icons::generic_bomb<colour::kCategoryCorruption>{});
  case mod_id::kCorruptionShield:
    return render(icons::generic_shield<colour::kCategoryCorruption>{});
  case mod_id::kCorruptionBonus:
    return render(icons::generic_bonus<colour::kCategoryCorruption>{});
  case mod_id::kCloseCombatWeapon:
    return render(icons::generic_weapon<colour::kCategoryCloseCombat>{});
  case mod_id::kCloseCombatSuper:
    return render(icons::generic_super<colour::kCategoryCloseCombat>{});
  case mod_id::kCloseCombatBomb:
    return render(icons::generic_bomb<colour::kCategoryCloseCombat>{});
  case mod_id::kCloseCombatShield:
    return render(icons::generic_shield<colour::kCategoryCloseCombat>{});
  case mod_id::kCloseCombatBonus:
    return render(icons::generic_bonus<colour::kCategoryCloseCombat>{});
  case mod_id::kLightningWeapon:
    return render(icons::generic_weapon<colour::kCategoryLightning>{});
  case mod_id::kLightningSuper:
    return render(icons::generic_super<colour::kCategoryLightning>{});
  case mod_id::kLightningBomb:
    return render(icons::generic_bomb<colour::kCategoryLightning>{});
  case mod_id::kLightningShield:
    return render(icons::generic_shield<colour::kCategoryLightning>{});
  case mod_id::kLightningBonus:
    return render(icons::generic_bonus<colour::kCategoryLightning>{});
  case mod_id::kSniperWeapon:
    return render(icons::generic_weapon<colour::kCategorySniper>{});
  case mod_id::kSniperSuper:
    return render(icons::generic_super<colour::kCategorySniper>{});
  case mod_id::kSniperBomb:
    return render(icons::generic_bomb<colour::kCategorySniper>{});
  case mod_id::kSniperShield:
    return render(icons::generic_shield<colour::kCategorySniper>{});
  case mod_id::kSniperBonus:
    return render(icons::generic_bonus<colour::kCategorySniper>{});
  case mod_id::kLaserWeapon:
    return render(icons::generic_weapon<colour::kCategoryLaser>{});
  case mod_id::kLaserSuper:
    return render(icons::generic_super<colour::kCategoryLaser>{});
  case mod_id::kLaserBomb:
    return render(icons::generic_bomb<colour::kCategoryLaser>{});
  case mod_id::kLaserShield:
    return render(icons::generic_shield<colour::kCategoryLaser>{});
  case mod_id::kLaserBonus:
    return render(icons::generic_bonus<colour::kCategoryLaser>{});
  case mod_id::kClusterWeapon:
    return render(icons::generic_weapon<colour::kCategoryCluster>{});
  case mod_id::kClusterSuper:
    return render(icons::generic_super<colour::kCategoryCluster>{});
  case mod_id::kClusterBomb:
    return render(icons::generic_bomb<colour::kCategoryCluster>{});
  case mod_id::kClusterShield:
    return render(icons::generic_shield<colour::kCategoryCluster>{});
  case mod_id::kClusterBonus:
    return render(icons::generic_bonus<colour::kCategoryCluster>{});
  }
  return render(icons::mod_unknown{});
}

// TODO: render warnings:
// - if an upgrade will replace another.
// - if an upgrade would have no effect? (e.g. bonus in multiplayer)
struct ModUpgrade : ecs::component {
  static constexpr std::int32_t kPanelPadding = 8;
  static constexpr std::uint32_t kTitleFontSize = 14;
  static constexpr std::uint32_t kSlotFontSize = 12;
  static constexpr std::uint32_t kDescriptionFontSize = 10;
  static constexpr std::uint32_t kAnimFrames = 40;
  static constexpr fixed kPanelHeight = 80;
  static constexpr fixed kPanelBorder = 48;
  static constexpr float kIconWidth = kPanelHeight.to_float() - 3 * kPanelPadding - kTitleFontSize;

  ModUpgrade(v0::mod_id id, upgrade_position position) : mod_id{id}, position{position} {}
  upgrade_position position = upgrade_position::kL0;
  v0::mod_id mod_id = v0::mod_id::kNone;
  std::uint32_t timer = 0;
  std::uint32_t icon_animation = 0;
  std::optional<std::uint32_t> highlight_animation;
  bool destroy = false;

  void update(ecs::handle h, SimInterface& sim) {
    auto r = panel_rect(sim);
    bool highlight = false;
    ++icon_animation;

    if (destroy) {
      timer = std::min(kAnimFrames, timer);
      timer && --timer;
      if (!timer) {
        h.emplace<Destroy>();
      }
    } else {
      ++timer;
      sim.index().iterate_dispatch<Player>(
          [&](Player& pc, PlayerLoadout& loadout, const Transform& transform) {
            bool valid = !pc.mod_upgrade_chosen && r.contains(transform.centre);
            highlight |= valid;
            if (valid && pc.is_clicking) {
              const auto& data = mod_lookup(mod_id);
              pc.mod_upgrade_chosen = true;
              loadout.add_mod(mod_id);
              auto c = mod_category_colour(data.category);
              auto e = sim.emit(resolve_key::local(pc.player_number));
              e.play(sound::kPowerupLife, transform.centre)
                  .rumble(pc.player_number, 16, .25f, .75f)
                  .explosion(to_float(transform.centre), colour::kWhite0, 8, std::nullopt, 2.f)
                  .explosion(to_float(transform.centre), c, 8, std::nullopt, 3.f)
                  .explosion(to_float(transform.centre), colour::kWhite0, 8, std::nullopt, 4.f);
              for (std::uint32_t i = 0; i < 16; ++i) {
                e.explosion(to_float(transform.centre) + from_polar(2 * i * pi<float> / 16, 64.f),
                            c, 8, to_float(transform.centre), 2.f);
              }
              h.emplace<Destroy>();
            }
          });
    }

    if (!highlight) {
      highlight_animation.reset();
    } else if (highlight_animation) {
      ++*highlight_animation;
    } else {
      highlight_animation = 0;
    }
  }

  void render(ecs::const_handle, std::vector<render::shape>&) const {}

  void render_panel(std::vector<render::combo_panel>& output, const SimInterface& sim) const {
    auto r = panel_rect(sim);
    auto panel_size = to_float(r.size);
    auto panel_position = to_float(r.position);
    auto anim_size = panel_size;
    anim_size.x *= ease_in_out_quart(static_cast<float>(timer) / kAnimFrames);
    anim_size.y = std::min(anim_size.x, anim_size.y);

    const auto& data = mod_lookup(mod_id);
    auto slot_category = ustring{mod_category_name(data.category)};
    auto slot_name = mod_slot_name(data.slot);
    if (!slot_category.empty() && !slot_name.empty()) {
      slot_category += ustring::ascii(" ");
    }
    slot_category += slot_name;

    auto c = mod_category_colour(data.category);
    cvec4 ch{c.x, 0.f, 1.f, 1.f};
    auto t = highlight_animation ? .5f + .5f * sin(*highlight_animation / (3.f * pi<float>)) : 0.f;

    render::combo_panel panel;
    panel.panel = {.style = render::panel_style::kFlatColour,
                   .colour = colour::kBlackOverlay0,
                   .border = glm::mix(colour::kPanelBorder0, colour::kWhite0, t),
                   .bounds = {panel_position, anim_size}};
    panel.padding = ivec2{kPanelPadding};
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {fvec2{0}, {panel_size.x - 2 * kPanelPadding, kTitleFontSize}},
        .e =
            render::combo_panel::text{
                .font = {render::font_id::kMonospaceBoldItalic, uvec2{kTitleFontSize}},
                .align = render::alignment::kLeft | render::alignment::kBottom,
                .colour = glm::mix(c, ch, t),
                .drop_shadow = {{}},
                .text = ustring{data.name},
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {fvec2{0}, {panel_size.x - 2 * kPanelPadding, kTitleFontSize}},
        .e =
            render::combo_panel::text{
                .font = {render::font_id::kMonospaceBold, uvec2{kSlotFontSize}},
                .align = render::alignment::kRight | render::alignment::kBottom,
                .colour = c,
                .drop_shadow = {{}},
                .text = slot_category,
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {{kIconWidth + kPanelPadding, kTitleFontSize + kPanelPadding},
                   {panel_size.x - 3 * kPanelPadding - kIconWidth,
                    panel_size.y - 3 * kPanelPadding - kTitleFontSize}},
        .e =
            render::combo_panel::text{
                .font = {render::font_id::kMonospace, uvec2{kDescriptionFontSize}},
                .align = render::alignment::kTop | render::alignment::kLeft,
                .colour = colour::kWhite0,
                .drop_shadow = {{}},
                .text = ustring{data.description},
                .multiline = true,
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {{0, kTitleFontSize + kPanelPadding},
                   {kIconWidth,
                    std::min(kIconWidth, panel_size.y - 3 * kPanelPadding - kTitleFontSize)}},
        .e = render::combo_panel::icon{.shapes = render_mod_icon(mod_id, icon_animation)},
    });
    output.emplace_back(std::move(panel));
  }

  rect panel_rect(const SimInterface& sim) const {
    vec2 panel_size{(sim.dimensions().x - 3 * kPanelBorder) / 2, kPanelHeight};
    vec2 panel_position{0};
    if (position == upgrade_position::kL0 || position == upgrade_position::kL1) {
      panel_position.x = kPanelBorder;
    } else {
      panel_position.x = sim.dimensions().x - kPanelBorder - panel_size.x;
    }
    if (position == upgrade_position::kL0 || position == upgrade_position::kR0) {
      panel_position.y = kPanelBorder;
    } else {
      panel_position.y = sim.dimensions().y - kPanelBorder - panel_size.y;
    }
    return {panel_position, panel_size};
  }
};

}  // namespace

void spawn_mod_upgrades(SimInterface& sim, std::span<mod_id> ids) {
  sim.index().iterate<Player>([&](Player& pc) { pc.mod_upgrade_chosen = false; });
  for (std::uint32_t i = 0; i < ids.size(); ++i) {
    auto h = sim.index().create();
    h.add(ModUpgrade{ids[i], upgrade_position{i}});
    h.add(Render{.render = sfn::cast<Render::render_t, ecs::call<&ModUpgrade::render>>,
                 .render_panel =
                     sfn::cast<Render::render_panel_t, ecs::call<&ModUpgrade::render_panel>>});
    h.add(Update{.update = ecs::call<&ModUpgrade::update>});
  }
}

void cleanup_mod_upgrades(SimInterface& sim) {
  sim.index().iterate<ModUpgrade>([&](ModUpgrade& upgrade) { upgrade.destroy = true; });
}

bool is_mod_upgrade_choice_done(const SimInterface& sim) {
  bool all_chosen = true;
  sim.index().iterate<Player>([&](const Player& pc) { all_chosen &= pc.mod_upgrade_chosen; });
  return all_chosen || !sim.index().count<ModUpgrade>();
}

}  // namespace ii::v0