#include "game/logic/v0/player/upgrade.h"
#include "game/common/colour.h"
#include "game/common/easing.h"
#include "game/core/icons/mod_icons.h"
#include "game/geometry/iteration.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/ship_template.h"
#include "game/logic/v0/player/loadout.h"
#include "game/logic/v0/player/loadout_mods.h"

namespace ii::v0 {
namespace {

enum class upgrade_position : std::uint32_t {
  kL0 = 0,
  kR0 = 1,
  kL1 = 2,
  kR1 = 3,
};

std::vector<render::shape>
render_mod_icon(const SimInterface& sim, const mod_data& data, std::uint32_t animation) {
  auto render = [&](geom2::ShapeBank::node_construct_t construct) {
    auto c = mod_category_colour(data.category);
    auto& r = local_resolve();
    geom2::resolve(r, sim.shape_bank(), construct, [&](geom2::parameter_set& parameters) {
      parameters.add(geom2::key{'r'}, static_cast<fixed>(animation) / 24)
          .add(geom2::key{'c'}, c)
          .add(geom2::key{'f'}, colour::alpha(c, colour::kFillAlpha0));
    });

    std::vector<render::shape> shapes;
    render_shape(shapes, r);
    return shapes;
  };

  switch (data.id) {
  case mod_id::kNone:
    break;
  case mod_id::kBackShots:
  case mod_id::kFrontShots:
  case mod_id::kBounceShots:
  case mod_id::kHomingShots:
    return render(&icons::generic_weapon);

  case mod_id::kSuperCapacity:
  case mod_id::kSuperRefill:
    return render(&icons::generic_super);

  case mod_id::kBombCapacity:
  case mod_id::kBombRadius:
  case mod_id::kBombSpeedClearCharge:
  case mod_id::kBombDoubleTrigger:
    return render(&icons::generic_bomb);

  case mod_id::kShieldCapacity:
  case mod_id::kShieldRefill:
  case mod_id::kShieldRespawn:
    return render(&icons::generic_shield);

  case mod_id::kPowerupDrops:
  case mod_id::kCurrencyDrops:
    return render(&icons::generic_bonus);

  case mod_id::kCorruptionWeapon:
  case mod_id::kCloseCombatWeapon:
  case mod_id::kLightningWeapon:
  case mod_id::kSniperWeapon:
  case mod_id::kLaserWeapon:
  case mod_id::kClusterWeapon:
    return render(&icons::generic_weapon);

  case mod_id::kCorruptionSuper:
  case mod_id::kCloseCombatSuper:
  case mod_id::kLightningSuper:
  case mod_id::kSniperSuper:
  case mod_id::kLaserSuper:
  case mod_id::kClusterSuper:
    return render(&icons::generic_super);

  case mod_id::kCorruptionBomb:
  case mod_id::kCloseCombatBomb:
  case mod_id::kLightningBomb:
  case mod_id::kSniperBomb:
  case mod_id::kLaserBomb:
  case mod_id::kClusterBomb:
    return render(&icons::generic_bomb);

  case mod_id::kCorruptionShield:
  case mod_id::kCloseCombatShield:
  case mod_id::kLightningShield:
  case mod_id::kSniperShield:
  case mod_id::kLaserShield:
  case mod_id::kClusterShield:
    return render(&icons::generic_shield);

  case mod_id::kCorruptionBonus:
  case mod_id::kCloseCombatBonus:
  case mod_id::kLightningBonus:
  case mod_id::kSniperBonus:
  case mod_id::kLaserBonus:
  case mod_id::kClusterBonus:
    return render(&icons::generic_bonus);
  }
  return render(&icons::mod_unknown);
}

// TODO: needs a better effect on pickup.
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
  v0::mod_id mod_id = v0::mod_id::kNone;
  upgrade_position position = upgrade_position::kL0;
  std::uint32_t timer = 0;
  std::uint32_t icon_animation = 0;
  std::optional<std::uint32_t> highlight_animation;
  bool destroy = false;

  void update(ecs::handle h, AiClickTag& click_tag, SimInterface& sim) {
    auto r = panel_rect(sim);
    click_tag.position = (r.min() + r.max()) / 2;
    bool highlight = false;
    ++icon_animation;

    if (destroy) {
      timer && --timer;
      if (!timer) {
        h.emplace<Destroy>();
      }
    } else {
      ++timer;
      sim.index().iterate_dispatch<Player>(
          [&](ecs::handle ph, Player& pc, PlayerLoadout& loadout, const Transform& transform) {
            bool valid = !pc.mod_upgrade_chosen && r.contains(transform.centre);
            highlight |= valid;
            if (!h.has<Destroy>() && valid && pc.is_clicking) {
              const auto& data = mod_lookup(mod_id);
              pc.mod_upgrade_chosen = true;
              timer = 2 * kAnimFrames;
              loadout.add(ph, mod_id, sim);
              auto c = mod_category_colour(data.category);
              auto e = sim.emit(resolve_key::local(pc.player_number));
              e.play(sound::kPowerupLife, transform.centre)
                  .rumble(pc.player_number, 16, .25f, .75f)
                  .explosion(to_float(transform.centre), colour::kWhite0, 24, std::nullopt, 1.f)
                  .explosion(to_float(transform.centre), c, 16, std::nullopt, 1.5f)
                  .explosion(to_float(transform.centre), colour::kWhite0, 8, std::nullopt, 2.f);
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
                .content = ustring{data.name},
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
                .content = slot_category,
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
                .content = ustring{data.description},
                .multiline = true,
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {{0, kTitleFontSize + kPanelPadding},
                   {kIconWidth,
                    std::min(kIconWidth, panel_size.y - 3 * kPanelPadding - kTitleFontSize)}},
        .e = render::combo_panel::icon{.shapes = render_mod_icon(sim, data, icon_animation)},
    });
    output.emplace_back(std::move(panel));

    sim.index().iterate_dispatch<Player>(
        [&](const Player& pc, const PlayerLoadout& loadout, const Transform& transform) {
          bool valid = !pc.mod_upgrade_chosen && r.contains(transform.centre);
          if (!valid) {
            return;
          }
          auto mod_valid = loadout.check_viability(mod_id);
          ustring warning_text = ustring::ascii("Unknown warning");
          switch (mod_valid.first) {
          case PlayerLoadout::viability::kOk:
            return;
          case PlayerLoadout::viability::kAlreadyHave:
            warning_text = ustring::ascii("Already present");
            break;
          case PlayerLoadout::viability::kNoEffectYet:
            warning_text = ustring::ascii("No effect yet");
            break;
          case PlayerLoadout::viability::kReplacesMod:
            warning_text = ustring::ascii("Replaces: ") + mod_lookup(mod_valid.second).name;
            break;
          }

          static constexpr std::int32_t kWarningPanelOffsetY = 32;
          fvec2 offset{0,
                       position == upgrade_position::kL0 || position == upgrade_position::kR0
                           ? kWarningPanelOffsetY
                           : -static_cast<float>(kWarningPanelOffsetY + kDescriptionFontSize +
                                                 2 * kPanelPadding)};
          frect bounds{to_float(transform.centre) + offset,
                       fvec2{0, kDescriptionFontSize + 2 * kPanelPadding}};
          output.emplace_back(render::combo_panel{
              .panel = {.style = render::panel_style::kFlatColour,
                        .colour = colour::kBlackOverlay0,
                        .border = colour::kPanelBorder0,
                        .bounds = bounds},
              .padding = ivec2{kPanelPadding},
              .elements = {{
                  .bounds = {fvec2{0}, fvec2{0, kDescriptionFontSize}},
                  .e =
                      render::combo_panel::text{
                          .font = {render::font_id::kMonospace, uvec2{kDescriptionFontSize}},
                          .align = render::alignment::kCentered,
                          .colour = colour::kSolarizedDarkYellow,
                          .drop_shadow = {{}},
                          .content = warning_text,
                      },
              }},
          });
        });
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
DEBUG_STRUCT_TUPLE(ModUpgrade, position, mod_id, timer, icon_animation, highlight_animation,
                   destroy);

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
    h.add(AiClickTag{});
  }
}

void cleanup_mod_upgrades(SimInterface& sim) {
  sim.index().iterate<ModUpgrade>([&](ModUpgrade& upgrade) {
    upgrade.timer = std::min(upgrade.timer, ModUpgrade::kAnimFrames);
    upgrade.destroy = true;
  });
}

bool is_mod_upgrade_choice_done(const SimInterface& sim) {
  bool all_chosen = true;
  sim.index().iterate<Player>([&](const Player& pc) { all_chosen &= pc.mod_upgrade_chosen; });
  return all_chosen || !sim.index().count<ModUpgrade>();
}

}  // namespace ii::v0
