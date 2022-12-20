#include "game/logic/v0/player/upgrade.h"
#include "game/common/easing.h"
#include "game/core/icons/mod_icons.h"
#include "game/logic/sim/sim_interface.h"
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

struct ModUpgrade : ecs::component {
  static constexpr std::int32_t kPanelPadding = 8;
  static constexpr std::uint32_t kTitleFontSize = 14;
  static constexpr std::uint32_t kSlotFontSize = 12;
  static constexpr std::uint32_t kDescriptionFontSize = 10;
  static constexpr std::uint32_t kAnimFrames = 40;
  static constexpr float kPanelHeight = 80.f;
  static constexpr float kPanelBorder = 48.f;
  static constexpr float kIconWidth = kPanelHeight - 3 * kPanelPadding - kTitleFontSize;

  ModUpgrade(v0::mod_id id, upgrade_position position) : mod_id{id}, position{position} {}
  upgrade_position position = upgrade_position::kL0;
  v0::mod_id mod_id = v0::mod_id::kNone;
  std::uint32_t timer = 0;
  bool destroy = false;

  void update(ecs::handle h, SimInterface&) {
    if (destroy) {
      timer = std::min(kAnimFrames, timer);
      timer && --timer;
      if (!timer) {
        h.emplace<Destroy>();
      }
    } else {
      ++timer;
    }
  }

  void render(ecs::const_handle h, std::vector<render::shape>& output) const {
    // TODO
  }

  void render_panel(std::vector<render::combo_panel>& output, const SimInterface& sim) const {
    fvec2 panel_size{(sim.dimensions().x.to_float() - 3 * kPanelBorder) / 2, kPanelHeight};
    fvec2 panel_position{0};
    if (position == upgrade_position::kL0 || position == upgrade_position::kL1) {
      panel_position.x = kPanelBorder;
    } else {
      panel_position.x = sim.dimensions().x.to_float() - kPanelBorder - panel_size.x;
    }
    if (position == upgrade_position::kL0 || position == upgrade_position::kR0) {
      panel_position.y = kPanelBorder;
    } else {
      panel_position.y = sim.dimensions().y.to_float() - kPanelBorder - panel_size.y;
    }

    const auto& data = mod_lookup(mod_id);
    auto slot_category = ustring{mod_category_name(data.category)};
    auto slot_name = mod_slot_name(data.slot);
    if (!slot_category.empty() && !slot_name.empty()) {
      slot_category += ustring::ascii(" ");
    }
    slot_category += slot_name;

    auto anim_size = panel_size;
    anim_size.x *= ease_in_out_quart(static_cast<float>(timer) / kAnimFrames);
    anim_size.y = std::min(anim_size.x, anim_size.y);

    render::combo_panel panel;
    panel.panel = {.style = render::panel_style::kFlatColour,
                   .colour = colour::kBlackOverlay0,
                   .border = colour::kPanelBorder0,
                   .bounds = {panel_position, anim_size}};
    panel.padding = ivec2{kPanelPadding};
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {fvec2{0}, fvec2{panel_size.x - 2 * kPanelPadding, kTitleFontSize}},
        .e =
            render::combo_panel::text{
                .font = {render::font_id::kMonospaceBoldItalic, uvec2{kTitleFontSize}},
                .align = render::alignment::kLeft | render::alignment::kBottom,
                .colour = mod_category_colour(data.category),
                .drop_shadow = {{}},
                .text = ustring{data.name},
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {fvec2{0}, fvec2{panel_size.x - 2 * kPanelPadding, kTitleFontSize}},
        .e =
            render::combo_panel::text{
                .font = {render::font_id::kMonospaceBold, uvec2{kSlotFontSize}},
                .align = render::alignment::kRight | render::alignment::kBottom,
                .colour = mod_category_colour(data.category),
                .drop_shadow = {{}},
                .text = slot_category,
            },
    });
    panel.elements.emplace_back(render::combo_panel::element{
        .bounds = {fvec2{kIconWidth + kPanelPadding, kTitleFontSize + kPanelPadding},
                   fvec2{panel_size.x - 3 * kPanelPadding - kIconWidth,
                         panel_size.y - 2 * kPanelPadding - kTitleFontSize}},
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
    output.emplace_back(std::move(panel));
  }
};

}  // namespace

void spawn_mod_upgrades(SimInterface& sim, std::span<mod_id> ids) {
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
  return !sim.index().count<ModUpgrade>() || /* TODO */ sim.tick_count() % 4096 == 0;
}

}  // namespace ii::v0