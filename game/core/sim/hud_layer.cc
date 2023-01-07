#include "game/core/sim/hud_layer.h"
#include "game/core/layers/common.h"
#include "game/core/toolkit/layout.h"
#include "game/core/toolkit/panel.h"
#include "game/core/toolkit/text.h"
#include "game/logic/sim/io/output.h"
#include "game/render/gl_renderer.h"

namespace ii {
inline std::string convert_to_time(std::uint64_t score) {
  if (!score) {
    return "--:--";
  }
  score /= 60u;
  std::uint64_t secs = score % 60u;
  score /= 60u;
  std::uint64_t mins = score % 60u;
  score /= 60u;

  std::string r;
  if (score) {
    r += std::to_string(score) + ":";
  }
  if (mins < 10u) {
    r += '0';
  }
  r += std::to_string(mins) + ":";
  if (secs < 10u) {
    r += '0';
  }
  r += std::to_string(secs);
  return r;
}

class BarFill : public ui::Element {
public:
  using Element::Element;
  render::panel_style style = render::panel_style::kNone;
  cvec4 colour = colour::kZero;
  float value = 0.f;
  float smooth_value = 0.f;

protected:
  void render_content(render::GlRenderer& r) const override {
    if (style == render::panel_style::kNone) {
      return;
    }
    auto x = static_cast<float>(bounds().size.x);
    r.render_panel({
        .style = style,
        .colour = colour::alpha(colour, 5.f / 8),
        .bounds = frect{bounds().size_rect()}.contract_max({x - x * value, 0}),
    });
    if (smooth_value > value) {
      r.render_panel({
          .style = style,
          .colour = colour::alpha(colour, 3.f / 8),
          .bounds = frect{bounds().size_rect()}
                        .contract_min({x * value, 0})
                        .contract_max({x - x * smooth_value, 0}),
      });
    }
  }
};

class PlayerHud : public ui::Panel {
public:
  PlayerHud(ui::Element* parent, render::alignment align) : ui::Panel(parent) {
    auto& layout = *add_back<ui::LinearLayout>();
    layout.set_spacing(kPadding.y);
    if (+(align & render::alignment::kTop)) {
      status_ = layout.add_back<ui::TextElement>();
      debug_ = layout.add_back<ui::TextElement>();
    } else {
      debug_ = layout.add_back<ui::TextElement>();
      status_ = layout.add_back<ui::TextElement>();
    }
    layout.set_align_end(!(align & render::alignment::kTop));
    layout.set_absolute_size(*status_, kMediumFont.y);

    status_->set_alignment(align)
        .set_font({render::font_id::kMonospace, kLargeFont})
        .set_colour(kTextColour);
    debug_->set_alignment(align)
        .set_font({render::font_id::kMonospace, kSmallFont})
        .set_colour(kTextColour)
        .set_multiline(true);
  }

  void set_colour(const cvec4& colour) { status_->set_colour(colour); }
  void set_player_status(ustring&& s) { status_->set_text(std::move(s)); }
  void set_debug_text(ustring&& s) { debug_->set_text(std::move(s)); }

private:
  ui::TextElement* status_ = nullptr;
  ui::TextElement* debug_ = nullptr;
};

HudLayer::HudLayer(ui::GameStack& stack, game_mode mode)
: ui::GameLayer{stack, ui::layer_flag::kCaptureCursor}, mode_{mode} {
  set_bounds(irect{kGameDimensions});

  static constexpr auto padding = 3u * kPadding;
  auto add = [&](const irect& r, render::alignment align) {
    auto& p = *add_back<PlayerHud>(align);
    p.set_margin(padding);
    p.set_bounds(r);
    return &p;
  };

  huds_ = {
      add(irect{ivec2{0}, kUiDimensions / 2}, render::alignment::kLeft | render::alignment::kTop),
      add(irect{ivec2{kUiDimensions.x / 2, 0}, kUiDimensions / 2},
          render::alignment::kRight | render::alignment::kTop),
      add(irect{ivec2{0, kUiDimensions.y / 2}, kUiDimensions / 2},
          render::alignment::kLeft | render::alignment::kBottom),
      add(irect{kUiDimensions / 2, kUiDimensions / 2},
          render::alignment::kRight | render::alignment::kBottom),
  };

  auto& p = *add_back<ui::Panel>();
  p.set_margin(padding);
  p.set_bounds(bounds().size_rect());
  status_ = p.add_back<ui::TextElement>();
  status_->set_alignment(render::alignment::kTop)
      .set_font({render::font_id::kMonospace, kMediumFont})
      .set_multiline(true);
  if (!is_legacy_mode(mode)) {
    status_->set_drop_shadow({});
  }

  static constexpr std::uint32_t kBarPadding = 2;
  static constexpr std::uint32_t kBarMarginX = 160;
  static constexpr std::uint32_t kBarMarginY = 20;
  static constexpr std::uint32_t kBarHeightY = 10;

  auto& e0 = *p.add_back<ui::Panel>();
  e0.set_margin({kBarMarginX, kBarMarginY});
  auto& e1 = *e0.add_back<ui::LinearLayout>();
  bar_text_ = e1.add_back<ui::TextElement>();
  bar_text_->set_alignment(render::alignment::kBottom | render::alignment::kLeft)
      .set_font({render::font_id::kMonospace, kMediumFont});
  if (!is_legacy_mode(mode)) {
    bar_text_->set_drop_shadow({});
  }
  bar_outer_ = e1.add_back<ui::Panel>();
  bar_outer_->set_padding(glm::ivec2{kBarPadding});
  bar_inner_ = bar_outer_->add_back<BarFill>();
  e1.set_orientation(ui::orientation::kVertical)
      .set_spacing(kBarPadding)
      .set_absolute_size(*bar_outer_, kBarHeightY);
}

void HudLayer::update_content(const ui::input_frame&, ui::output_frame&) {
  if (!render_) {
    return;
  }
  for (std::size_t i = 0; i < 4; ++i) {
    if (i >= render_->players.size()) {
      huds_[i]->set_player_status({});
      huds_[i]->set_debug_text({});
      continue;
    }
    huds_[i]->set_debug_text(ustring::ascii(debug_text_[i]));
    if (mode_ == game_mode::kStandardRun) {
      continue;
    }
    const auto& p = render_->players[i];
    huds_[i]->set_colour(p.colour);
    huds_[i]->set_player_status(
        ustring::ascii(std::to_string(p.score) + " (" + std::to_string(p.multiplier) + "X)"));
  }

  std::string s;
  if (mode_ == game_mode::kStandardRun) {
    s = convert_to_time(render_->tick_count);
    if (render_->overmind_wave) {
      s += " (wave " + std::to_string(*render_->overmind_wave) + ")";
    }
  } else {
    s = std::to_string(render_->lives_remaining) + " live(s)";
    if (mode_ == game_mode::kLegacy_Boss) {
      s += "\n" + convert_to_time(render_->tick_count);
    } else if (render_->overmind_timer) {
      auto t = *render_->overmind_timer / 60;
      s += (t < 10 ? " 0" : " ") + std::to_string(t);
    }
  }
  if (!status_text_.empty()) {
    s = status_text_ + "\n" + s;
  }
  if (!render_->debug_text.empty() && stack().options().debug) {
    s += "\n" + render_->debug_text;
  }
  status_->set_text(ustring::ascii(s));

  static constexpr std::uint32_t kBossBarTimer = 60;
  static constexpr float kBossBarDelta = 1.f / 2048;
  if (render_->boss) {
    if (!boss_bar_) {
      boss_bar_.emplace();
      boss_bar_->smooth_value = render_->boss->hp_bar;
    }
    boss_bar_->timer = kBossBarTimer;
    boss_bar_->colour = render_->boss->colour;
    boss_bar_->style = render::panel_style::kFlatColour;
    boss_bar_->name = render_->boss->name;
    boss_bar_->value = std::min(boss_bar_->value, render_->boss->hp_bar);
    if (boss_bar_->value < render_->boss->hp_bar) {
      boss_bar_->value =
          std::min(render_->boss->hp_bar,
                   std::max(boss_bar_->value + kBossBarDelta,
                            rc_smooth(boss_bar_->value, render_->boss->hp_bar, 31.f / 32)));
    }
    boss_bar_->smooth_value = std::max(boss_bar_->smooth_value, boss_bar_->value);
    if (boss_bar_->smooth_value > boss_bar_->value) {
      boss_bar_->smooth_value =
          std::max(boss_bar_->value,
                   std::min(boss_bar_->smooth_value - kBossBarDelta,
                            rc_smooth(boss_bar_->smooth_value, boss_bar_->value, 31.f / 32)));
    }
  } else {
    if (boss_bar_) {
      boss_bar_->value = 0.f;
      boss_bar_->smooth_value =
          std::max(0.f,
                   std::min(boss_bar_->smooth_value - kBossBarDelta,
                            rc_smooth(boss_bar_->smooth_value, 0.f, 31.f / 32)));
      boss_bar_->timer && --boss_bar_->timer;
      if (!boss_bar_->timer) {
        boss_bar_.reset();
      }
    }
  }
  if (boss_bar_) {
    bar_inner_->value = boss_bar_->value;
    bar_inner_->smooth_value = boss_bar_->smooth_value;
    bar_inner_->style = boss_bar_->style;
    bar_inner_->colour = boss_bar_->colour;
    bar_outer_->set_style(render::panel_style::kFlatColour)
        .set_colour(colour::kBlackOverlay0)
        .set_border(colour::kWhite0);
    bar_text_->set_text(ustring{boss_bar_->name});
  } else {
    bar_outer_->set_style(render::panel_style::kNone);
    bar_inner_->style = render::panel_style::kNone;
    bar_text_->set_text({});
  }
}

void HudLayer::render_content(render::GlRenderer&) const {
  // TODO: hp bar.
}

}  // namespace ii
